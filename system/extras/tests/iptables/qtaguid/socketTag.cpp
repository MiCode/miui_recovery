/*
 * Copyright (C) 2011 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless requied by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 */
/*
 * This socket tagging test is to ensure that the
 * netfilter/xt_qtaguid kernel module somewhat behaves as expected
 * with respect to tagging sockets.
 */
#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <string>

#define LOG_TAG "socketTagTest"
#include <utils/Log.h>
#include <testUtil.h>


class SockInfo {
public:
        SockInfo()
                : fd(-1), addr(NULL) {};
        int setup(uint64_t tag);
        bool checkTag(uint64_t tag, uid_t uid);
        int fd;
        void *addr;
};


int openCtrl() {
    int ctrl;
    ctrl = open("/proc/net/xt_qtaguid/ctrl", O_RDWR);
    if (!ctrl) {
       testPrintE("qtaguid ctrl open failed: %s", strerror(errno));
    }
    return ctrl;
}

int doCtrlCommand(const char *fmt, ...) {
    char *buff;
    int ctrl;
    int res;
    va_list argp;

    va_start(argp, fmt);
    ctrl = openCtrl();
    vasprintf(&buff, fmt, argp);
    errno = 0;
    res = write(ctrl, buff, strlen(buff));
    testPrintI("cmd: '%s' res=%d %d/%s", buff, res, errno, strerror(errno));
    close(ctrl);
    free(buff);
    va_end(argp);
    return res;
}


int writeModuleParam(const char *param, const char *data) {
    int param_fd;
    int res;
    std::string filename("/sys/module/xt_qtaguid/parameters/");

    filename += param;
    param_fd = open(filename.c_str(), O_WRONLY);
    if (param_fd < 0) {
        testPrintE("qtaguid param open failed: %s", strerror(errno));
        return -1;
    }
    res = write(param_fd, data, strlen(data));
    if (res < 0) {
        testPrintE("qtaguid param write failed: %s", strerror(errno));
    }
    close(param_fd);
    return res;
}
/*----------------------------------------------------------------*/
int SockInfo::setup(uint64_t tag) {
    fd = socket(AF_INET, SOCK_STREAM, 0);
    if (fd < 0) {
        testPrintE("socket creation failed: %s", strerror(errno));
        return -1;
    }
    if (doCtrlCommand("t %d %llu", fd, tag) < 0) {
        testPrintE("socket setup: failed to tag");
        close(fd);
        return -1;
    }
    if (!checkTag(tag, getuid())) {
        testPrintE("socket setup: Unexpected results: tag not found");
        close(fd);
        return -1;
    }
    if (doCtrlCommand("u %d", fd) < 0) {
        testPrintE("socket setup: Unexpected results");
        close(fd);
        return -1;
    }
    return 0;
}

/* checkTag() also tries to lookup the socket address in the kernel and
 * return it when *addr  == NULL.
 * This allows for better look ups when another process is also setting the same
 * tag + uid. But it is not fool proof.
 * Without the kernel reporting more info on who setup the socket tag, it is
 * not easily verifiable from user-space.
 * Returns: true if tag found.
 */
bool SockInfo::checkTag(uint64_t acct_tag, uid_t uid) {
    int ctrl_fd;
    ctrl_fd = openCtrl();
    char ctrl_data[1024];
    ssize_t read_size;
    char *buff;
    char *pos;
    int res;
    char *match_template;
    uint64_t k_tag;
    uint32_t k_uid;
    uint64_t full_tag;
    long dummy_count;
    pid_t dummy_pid;

    read_size = read(ctrl_fd, ctrl_data, sizeof(ctrl_data));
    if (read_size < 0) {
       testPrintE("Unable to read active tags from ctrl %d/%s",
                  errno, strerror(errno));
    }
    ctrl_data[read_size] = '\0';
    testPrintI("<ctrl_raw_data>\n%s</ctrl_raw_data>", ctrl_data);

    if (addr) {
        assert(sizeof(void*) == sizeof(long int));  // Why does %p use 0x? grrr. %lx.
        asprintf(&match_template, "sock=%lx %s", addr, "tag=0x%llx (uid=%u)");
    }
    else {
        /* Allocate for symmetry */
        asprintf(&match_template, "%s", " tag=0x%llx (uid=%u)");
    }

    full_tag = acct_tag | uid;

    asprintf(&buff, match_template, full_tag | uid, uid);
    testPrintI("looking for '%s'", buff);
    pos = strstr(ctrl_data, buff);

    if (pos && !addr) {
        assert(sizeof(void*) == sizeof(long int));  // Why does %p use 0x? grrr. %lx.
        res = sscanf(pos - strlen("sock=1234abcd"),
                     "sock=%lx tag=0x%llx (uid=%lu) pid=%u f_count=%lu",
                     &addr, &k_tag, &k_uid, &dummy_pid, &dummy_count );
        if (!(res == 5 && k_tag == full_tag && k_uid == uid)) {
            testPrintE("Unable to read sock addr res=%d", res);
           addr = 0;
        }
        else {
            testPrintI("Got sock_addr %lx", addr);
        }
    }
    free(buff);
    free(match_template);
    close(ctrl_fd);
    return pos != NULL;
}

/*----------------------------------------------------------------*/
int testSocketTagging(void) {
    SockInfo sock0;
    SockInfo sock1;
    int res;
    int total_errors = 0;
    int ctrl_fd = -1;
    int dev_fd = -1;
    const uint64_t invalid_tag1 = 0x0000000100000001llu;
    uint64_t valid_tag1;
    uint64_t valid_tag2;
    uint64_t max_uint_tag = 0xffffffff00000000llu;
    uid_t fake_uid;
    uid_t fake_uid2;
    const char *test_name;
    uid_t my_uid = getuid();
    pid_t my_pid = getpid();

    srand48(my_pid * my_uid);
    /* Adjust fake UIDs and tags so that multiple instances can run in parallel. */
    fake_uid = testRand();
    fake_uid2 = testRand();
    valid_tag1 = ((uint64_t)my_pid << 48) | ((uint64_t)testRand() << 32);
    valid_tag2 = ((uint64_t)my_pid << 48) | ((uint64_t)testRand() << 32);
    max_uint_tag = 1llu << 63 | (((uint64_t)my_pid << 48) ^ max_uint_tag);
    testSetLogCatTag(LOG_TAG);

    testPrintI("** %s ** ============================", __FUNCTION__);
    testPrintI("* start: pid=%lu uid=%lu uid1=0x%lx/%lu uid2=0x%lx/%lu"
               " tag1=0x%llx/%llu tag2=0x%llx/%llu",
               my_pid, my_uid, fake_uid, fake_uid, fake_uid2, fake_uid2,
               valid_tag1, valid_tag1, valid_tag2, valid_tag2);

    // ---------------
    test_name = "kernel has qtaguid";
    testPrintI("* test: %s ", test_name);
    ctrl_fd = openCtrl();
    if (ctrl_fd < 0) {
        testPrintE("qtaguid ctrl open failed: %s", strerror(errno));
        total_errors++;
        goto done;
    }
    close(ctrl_fd);
    dev_fd = open("/dev/xt_qtaguid", O_RDONLY);
    if (dev_fd < 0) {
        testPrintE("qtaguid dev open failed: %s", strerror(errno));
        total_errors++;
    }
    // ---------------
    test_name = "delete command doesn't fail";
    testPrintI("* test: %s", test_name);
    res = doCtrlCommand("d 0 %u", fake_uid);
    if (res < 0) {
            testPrintE("! %s: Unexpected results", test_name);
            total_errors++;
    }

    res = doCtrlCommand("d 0 %u", fake_uid2);
    if (res < 0) {
            testPrintE("! %s: Unexpected results", test_name);
            total_errors++;
    }

    res = doCtrlCommand("d 0 %u", my_uid);
    if (res < 0) {
            testPrintE("! %s: Unexpected results", test_name);
            total_errors++;
    }
    // ---------------
    test_name = "setup sock0 and addr via tag";
    testPrintI("* test: %s", test_name);
    if (sock0.setup(valid_tag1) < 0) {
        testPrintE("socket setup failed: %s", strerror(errno));
        total_errors++;
        goto done;

    }
    // ---------------
    test_name = "setup sock1 and addr via tag";
    testPrintI("* test: %s", test_name);
    if (sock1.setup(valid_tag1) < 0) {
        testPrintE("socket setup failed: %s", strerror(errno));
        total_errors++;
        goto done;
    }
    // ---------------
    test_name = "insufficient args. Expected failure";
    testPrintI("* test: %s", test_name);
    res = doCtrlCommand("t");
    if (res > 0) {
        testPrintE("! %s: Unexpected results", test_name);
        total_errors++;
    }

    // ---------------
    test_name = "bad command. Expected failure";
    testPrintI("* test: %s", test_name);
    res = doCtrlCommand("?");
    if (res > 0) {
            testPrintE("! %s: Unexpected results", test_name);
        total_errors++;
    }

    // ---------------
    test_name = "no tag, no uid";
    testPrintI("* test: %s", test_name);
    res = doCtrlCommand("t %d", sock0.fd);
    if (res < 0) {
        testPrintE("! %s: Unexpected results", test_name);
        total_errors++;
    }
    if (!sock0.checkTag(0, my_uid)) {
        testPrintE("! %s: Unexpected results: tag not found", test_name);
        total_errors++;
    }

    // ---------------
    test_name = "invalid tag. Expected failure";
    testPrintI("* test: %s", test_name);
    res = doCtrlCommand("t %d %llu", sock0.fd, invalid_tag1);
    if (res > 0) {
        testPrintE("! %s: Unexpected results", test_name);
        total_errors++;
    }
    if (sock0.checkTag(invalid_tag1, my_uid)) {
        testPrintE("! %s: Unexpected results: tag should not be there", test_name);
        total_errors++;
    }

    // ---------------
    test_name = "valid tag with no uid";
    testPrintI("* test: %s", test_name);
    res = doCtrlCommand("t %d %llu", sock0.fd, valid_tag1);
    if (res < 0) {
        testPrintE("! %s: Unexpected results", test_name);
        total_errors++;
    }
    if (!sock0.checkTag(valid_tag1, my_uid)) {
        testPrintE("! %s: Unexpected results: tag not found", test_name);
        total_errors++;
    }
    // ---------------
    test_name = "valid untag";
    testPrintI("* test: %s", test_name);
    res = doCtrlCommand("u %d", sock0.fd);
    if (res < 0) {
        testPrintE("! %s: Unexpected results", test_name);
        total_errors++;
    }
    if (sock0.checkTag(valid_tag1, my_uid)) {
        testPrintE("! %s: Unexpected results: tag not removed", test_name);
        total_errors++;
    }

    // ---------------
    test_name = "valid 1st tag";
    testPrintI("* test: %s", test_name);
    res = doCtrlCommand("t %d %llu %u", sock0.fd, valid_tag2, fake_uid);
    if (res < 0) {
        testPrintE("! %s: Unexpected results", test_name);
        total_errors++;
    }
    if (!sock0.checkTag(valid_tag2, fake_uid)) {
        testPrintE("! %s: Unexpected results: tag not found", test_name);
        total_errors++;
    }

    // ---------------
    test_name = "valid re-tag";
    testPrintI("* test: %s", test_name);
    res = doCtrlCommand("t %d %llu %u", sock0.fd, valid_tag2, fake_uid);
    if (res < 0) {
        testPrintE("! %s: Unexpected results", test_name);
        total_errors++;
    }
    if (!sock0.checkTag(valid_tag2, fake_uid)) {
        testPrintE("! %s: Unexpected results: tag not found", test_name);
        total_errors++;
    }

    // ---------------
    test_name = "valid re-tag with acct_tag change";
    testPrintI("* test: %s", test_name);
    res = doCtrlCommand("t %d %llu %u", sock0.fd, valid_tag1, fake_uid);
    if (res < 0) {
        testPrintE("! %s: Unexpected results", test_name);
        total_errors++;
    }
    if (!sock0.checkTag(valid_tag1, fake_uid)) {
        testPrintE("! %s: Unexpected results: tag not found", test_name);
        total_errors++;
    }

    // ---------------
    test_name = "re-tag with uid change";
    testPrintI("* test: %s", test_name);
    res = doCtrlCommand("t %d %llu %u", sock0.fd, valid_tag2, fake_uid2);
    if (res < 0) {
        testPrintE("! %s: Unexpected results", test_name);
        total_errors++;
    }
    if (!sock0.checkTag(valid_tag2, fake_uid2)) {
        testPrintE("! %s: Unexpected results: tag not found", test_name);
        total_errors++;
    }

    // ---------------
    test_name = "valid 64bit acct tag";
    testPrintI("* test: %s", test_name);
    res = doCtrlCommand("t %d %llu", sock0.fd, max_uint_tag);
    if (res < 0) {
        testPrintE("! %s: Unexpected results", test_name);
        total_errors++;
    }
    if (!sock0.checkTag(max_uint_tag, my_uid)) {
        testPrintE("! %s: Unexpected results: tag not found", test_name);
        total_errors++;
    }

    // ---------------
    test_name = "tag another socket";
    testPrintI("* test: %s", test_name);
    res = doCtrlCommand("t %d %llu %u", sock1.fd, valid_tag1, fake_uid2);
    if (res < 0) {
        testPrintE("! %s: Unexpected results", test_name);
        total_errors++;
    }
    if (!sock1.checkTag(valid_tag1, fake_uid2)) {
        testPrintE("! %s: Unexpected results: tag not found", test_name);
        total_errors++;
    }

    // ---------------
    test_name = "valid untag";
    testPrintI("* test: %s", test_name);
    res = doCtrlCommand("u %d", sock0.fd);
    if (res < 0) {
        testPrintE("! %s: Unexpected results", test_name);
        total_errors++;
    }
    if (sock0.checkTag(max_uint_tag, fake_uid)) {
        testPrintE("! %s: Unexpected results: tag should not be there", test_name);
        total_errors++;
    }
    if (!sock1.checkTag(valid_tag1, fake_uid2)) {
        testPrintE("! %s: Unexpected results: tag not found", test_name);
        total_errors++;
    }
    res = doCtrlCommand("u %d", sock1.fd);
    if (res < 0) {
        testPrintE("! %s: Unexpected results", test_name);
        total_errors++;
    }
    if (sock1.checkTag(valid_tag1, fake_uid2)) {
        testPrintE("! %s: Unexpected results: tag should not be there", test_name);
        total_errors++;
    }

    // ---------------
    test_name = "invalid sock0.fd. Expected failure";
    testPrintI("* test: %s", test_name);
    close(sock0.fd);
    res = doCtrlCommand("t %d %llu %u", sock0.fd, valid_tag1, my_uid);
    if (res > 0) {
            testPrintE("! %s: Unexpected results", test_name);
        total_errors++;
    }
    if (sock0.checkTag(valid_tag1, my_uid)) {
        testPrintE("! %s: Unexpected results: tag should not be there", test_name);
        total_errors++;
    }

    // ---------------
    test_name = "invalid untag. Expected failure";
    testPrintI("* test: %s", test_name);
    close(sock1.fd);
    res = doCtrlCommand("u %d", sock1.fd);
    if (res > 0) {
        testPrintE("! %s: Unexpected results", test_name);
        total_errors++;
    }

    if (total_errors) {
        testPrintE("! Errors found");
    } else {
        testPrintI("No Errors found");
    }

done:
    if (dev_fd >= 0) {
        close(dev_fd);
    }
    if (ctrl_fd >= 0) {
        close(ctrl_fd);
    }
    return total_errors;
}

int testTagData(void) {
    SockInfo sock0;
    SockInfo sock1;
    int res;
    int total_errors = 0;
    int ctrl_fd = -1;
    int dev_fd = -1;
    const uint64_t invalid_tag1 = 0x0000000100000001llu;
    uint64_t valid_tag1;
    uint64_t valid_tag2;
    uint64_t max_uint_tag = 0xffffffff00000000llu;
    uid_t fake_uid;
    uid_t fake_uid2;
    const char *test_name;
    uid_t my_uid = getuid();
    pid_t my_pid = getpid();
    const int max_tags = 5;

    srand48(my_pid * my_uid);
    /* Adjust fake UIDs and tags so that multiple instances can run in parallel. */
    fake_uid = testRand();
    fake_uid2 = testRand();
    valid_tag1 = ((uint64_t)my_pid << 48) | ((uint64_t)testRand() << 32);
    valid_tag2 = ((uint64_t)my_pid << 48) | ((uint64_t)testRand() << 32);
    valid_tag2 &= 0xffffff00ffffffffllu;  // Leave some room to make counts visible.
    testSetLogCatTag(LOG_TAG);

    testPrintI("** %s ** ============================", __FUNCTION__);
    testPrintI("* start: pid=%lu uid=%lu uid1=0x%lx/%lu uid2=0x%lx/%lu"
               " tag1=0x%llx/%llu tag2=0x%llx/%llu",
               my_pid, my_uid, fake_uid, fake_uid, fake_uid2, fake_uid2,
               valid_tag1, valid_tag1, valid_tag2, valid_tag2);

    // ---------------
    test_name = "kernel has qtaguid";
    testPrintI("* test: %s ", test_name);
    ctrl_fd = openCtrl();
    if (ctrl_fd < 0) {
        testPrintE("qtaguid ctrl open failed: %s", strerror(errno));
        return 1;
    }
    close(ctrl_fd);
    dev_fd = open("/dev/xt_qtaguid", O_RDONLY);
    if (dev_fd < 0) {
        testPrintE("! %s: qtaguid dev open failed: %s", test_name, strerror(errno));
        total_errors++;
    }
    // ---------------
    test_name = "delete command doesn't fail";
    testPrintI("* test: %s", test_name);
    res = doCtrlCommand("d 0 %u", fake_uid);
    if (res < 0) {
            testPrintE("! %s: Unexpected results", test_name);
            total_errors++;
    }

    res = doCtrlCommand("d 0 %u", fake_uid2);
    if (res < 0) {
            testPrintE("! %s: Unexpected results", test_name);
            total_errors++;
    }

    res = doCtrlCommand("d 0 %u", my_uid);
    if (res < 0) {
            testPrintE("! %s: Unexpected results", test_name);
            total_errors++;
    }

    // ---------------
    test_name = "setup sock0 and addr via tag";
    testPrintI("* test: %s", test_name);
    if (sock0.setup(valid_tag1)) {
        testPrintE("socket setup failed: %s", strerror(errno));
        return 1;
    }
    // ---------------
    test_name = "setup sock1 and addr via tag";
    testPrintI("* test: %s", test_name);
    if (sock1.setup(valid_tag1)) {
        testPrintE("socket setup failed: %s", strerror(errno));
        return 1;
    }
    // ---------------
    test_name = "setup tag limit";
    testPrintI("* test: %s ", test_name);
    char *max_tags_str;
    asprintf(&max_tags_str, "%d", max_tags);
    res = writeModuleParam("max_sock_tags", max_tags_str);
    if (res < 0) {
        testPrintE("! %s: Unexpected results", test_name);
        free(max_tags_str);
        return 1;
    }

    // ---------------
    test_name = "tag quota reach limit";
    testPrintI("* test: %s", test_name);
    for (int cnt = 0; cnt < max_tags; cnt++ ) {
        uint64_t new_tag = valid_tag2 + ((uint64_t)cnt << 32);
        res = doCtrlCommand("t %d %llu %u", sock0.fd, new_tag , fake_uid2);
        if (res < 0) {
                testPrintE("! %s: Unexpected results", test_name);
                total_errors++;
        }
        if (!sock0.checkTag(new_tag, fake_uid2)) {
                testPrintE("! %s: Unexpected results: tag not found", test_name);
                total_errors++;
        }
    }
    test_name = "tag quota go over limit";
    testPrintI("* test: %s", test_name);
    {
        uint64_t new_tag = valid_tag2 + ((uint64_t)max_tags << 32);
        res = doCtrlCommand("t %d %llu %u", sock0.fd, new_tag , fake_uid2);
        if (res > 0) {
                testPrintE("! %s: Unexpected results", test_name);
                total_errors++;
        }
        if (!sock0.checkTag(valid_tag2 + (((uint64_t)max_tags - 1) << 32), fake_uid2)) {
                testPrintE("! %s: Unexpected results: tag not found", test_name);
                total_errors++;
        }
    }

    // ---------------
    test_name = "valid untag";
    testPrintI("* test: %s", test_name);
    res = doCtrlCommand("u %d", sock0.fd);
    if (res < 0) {
        testPrintE("! %s: Unexpected results", test_name);
        total_errors++;
    }
    if (sock0.checkTag(valid_tag2 + (((uint64_t)max_tags - 1) << 32), fake_uid2)) {
        testPrintE("! %s: Unexpected results: tag should not be there", test_name);
        total_errors++;
    }

    // ---------------
    test_name = "tag after untag shouldn't free up max tags";
    testPrintI("* test: %s", test_name);
    {
        uint64_t new_tag = valid_tag2 + ((uint64_t)max_tags << 32);
        res = doCtrlCommand("t %d %llu %u", sock0.fd, new_tag , fake_uid2);
        if (res > 0) {
                testPrintE("! %s: Unexpected results", test_name);
                total_errors++;
        }
        if (sock0.checkTag(valid_tag2 + ((uint64_t)max_tags << 32), fake_uid2)) {
            testPrintE("! %s: Unexpected results: tag should not be there", test_name);
            total_errors++;
        }
    }

    // ---------------
    test_name = "delete one tag";
    testPrintI("* test: %s", test_name);
    {
            uint64_t new_tag = valid_tag2 + (((uint64_t)max_tags / 2) << 32);
            res = doCtrlCommand("d %llu %u", new_tag, fake_uid2);
            if (res < 0) {
                    testPrintE("! %s: Unexpected results", test_name);
                    total_errors++;
            }
    }

    // ---------------
    test_name = "2 tags after 1 delete pass/fail";
    testPrintI("* test: %s", test_name);
    {
        uint64_t new_tag;
        new_tag = valid_tag2 + (((uint64_t)max_tags + 1 ) << 32);
        res = doCtrlCommand("t %d %llu %u", sock0.fd, new_tag , fake_uid2);
        if (res < 0) {
                testPrintE("! %s: Unexpected results", test_name);
                total_errors++;
        }
        if (!sock0.checkTag(valid_tag2 + (((uint64_t)max_tags + 1) << 32), fake_uid2)) {
            testPrintE("! %s: Unexpected results: tag not found", test_name);
            total_errors++;
        }

        new_tag = valid_tag2 + (((uint64_t)max_tags + 2 ) << 32);
        res = doCtrlCommand("t %d %llu %u", sock0.fd, new_tag , fake_uid2);
        if (res > 0) {
                testPrintE("! %s: Unexpected results", test_name);
                total_errors++;
        }
        if (sock0.checkTag(valid_tag2 + (((uint64_t)max_tags + 2) << 32), fake_uid2)) {
            testPrintE("! %s: Unexpected results: tag should not be there", test_name);
            total_errors++;
        }
    }

    /* TODO(jpa): test tagging two different sockets with same tags and
     * check refcounts  the tag_node should be +2
     */

    // ---------------
    if (total_errors) {
        testPrintE("! Errors found");
    } else {
        testPrintI("No Errors found");
    }

done:
    if (dev_fd >= 0) {
        close(dev_fd);
    }
    if (ctrl_fd >= 0) {
        close(ctrl_fd);
    }
    return total_errors;
}

/*----------------------------------------------------------------*/

int main(int argc, char *argv[]) {
    int res = 0;
    res += testTagData();
    res += testSocketTagging();
    if (res) {
        testPrintE("!! %d Errors found", res);
    } else {
        testPrintE("No Errors found");
    }
    return res;
}
