/*
 * Copyright (C) 2011 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
package com.android.commands.sendbug;

import android.accounts.Account;
import android.accounts.IAccountManager;
import android.app.ActivityManagerNative;
import android.app.IActivityManager;
import android.content.Context;
import android.content.Intent;
import android.content.pm.IPackageManager;
import android.content.pm.ResolveInfo;
import android.net.Uri;
import android.os.RemoteException;
import android.os.ServiceManager;
import android.os.SystemProperties;

import java.io.File;
import java.util.ArrayList;
import java.util.List;

public class SendBug {

    private static final String GOOGLE_ACCOUNT_TYPE = "com.google";
    private static final String EMAIL_ACCOUNT_TYPE = "com.android.email";
    private static final String SEND_BUG_INTENT_ACTION = "android.testing.SEND_BUG";

    public static void main(String[] args) {
        if (args.length == 1) {
            new SendBug().run(args[0]);
        } else if (args.length == 2) {
            new SendBug().run(args[0], args[1]);
        }
    }

    private void run(String bugreportPath) {
        run(bugreportPath, null);
    }

    private void run(String bugreportPath, String screenShotPath) {
        final File bugreport = new File(bugreportPath);
        File screenShot = null;
        if (screenShotPath != null) {
            screenShot = new File(screenShotPath);
            if (!screenShot.exists()) {
              // screen shot probably failed
              screenShot = null;
            }
        }
        if (bugreport.exists()) {
            final Uri bugreportUri = Uri.fromFile(bugreport);
            // todo (aalbert): investigate adding a screenshot to BugReporter
            Intent intent = tryBugReporter(bugreportUri);
            if (intent == null) {
                final Uri screenshotUri = screenShot != null
                        ? Uri.fromFile(screenShot) : null;
                intent = getSendMailIntent(bugreportUri, screenshotUri);
            }
            final IActivityManager mAm = ActivityManagerNative.getDefault();
            try {
                mAm.startActivity(null, intent, intent.getType(), null, 0, null, null, 0, false,
                        false, null, null, false);
            } catch (RemoteException e) {
                // ignore
            }
        }
    }

    private Intent tryBugReporter(Uri bugreportUri) {
        final Intent intent = new Intent(SEND_BUG_INTENT_ACTION);
        intent.setData(bugreportUri);
        final IPackageManager mPm = IPackageManager.Stub.asInterface(
                ServiceManager.getService("package"));
        if (mPm != null) {
            final List<ResolveInfo> results;
            try {
                results = mPm.queryIntentActivities(intent, null, 0);
            } catch (RemoteException e) {
                return null;
            }
            if (results != null && results.size() > 0) {
                final ResolveInfo info = results.get(0);
                intent.addFlags(Intent.FLAG_ACTIVITY_NEW_TASK);
                intent.setClassName(info.activityInfo.applicationInfo.packageName,
                        info.activityInfo.name);
                return intent;
            } else {
                return null;
            }
        }
        return null;
    }

    private Intent getSendMailIntent(Uri bugreportUri, Uri screenshotUri) {
        final Account sendToAccount = findSendToAccount();
        final Intent intent = new Intent(Intent.ACTION_SEND);
        intent.addFlags(Intent.FLAG_ACTIVITY_NEW_TASK);
        intent.setType("application/octet-stream");
        intent.putExtra("subject", bugreportUri.getLastPathSegment());
        final StringBuilder sb = new StringBuilder();
        sb.append(SystemProperties.get("ro.build.description"));
        sb.append("\n(Sent from BugMailer)");
        intent.putExtra("body", sb.toString());
        if (screenshotUri != null) {
            final ArrayList<Uri> attachments = new ArrayList<Uri>();
            attachments.add(bugreportUri);
            attachments.add(screenshotUri);
            intent.setAction(Intent.ACTION_SEND_MULTIPLE);
            intent.putParcelableArrayListExtra(Intent.EXTRA_STREAM, attachments);
        } else {
            intent.putExtra(Intent.EXTRA_STREAM, bugreportUri);
        }
        if (sendToAccount != null) {
            intent.putExtra("to", sendToAccount.name);
        }
        return intent;
    }

    private Account findSendToAccount() {
        final IAccountManager accountManager = IAccountManager.Stub.asInterface(ServiceManager
                .getService(Context.ACCOUNT_SERVICE));
        Account[] accounts = null;
        Account foundAccount = null;
        try {
            accounts = accountManager.getAccounts(null);
        } catch (RemoteException e) {
            // ignore
        }
        if (accounts != null) {
            for (Account account : accounts) {
                if (GOOGLE_ACCOUNT_TYPE.equals(account.type)) {
                    // return first gmail account found
                    return account;
                } else if (EMAIL_ACCOUNT_TYPE.equals(account.type)) {
                    // keep regular email account for now in case there are gmail accounts
                    // found later
                    foundAccount = account;
                }
            }
        }
        return foundAccount;
    }

}
