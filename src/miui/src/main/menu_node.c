/*
 * Copyright (C) 2012 xiaomi MIUI ( http://www.xiaomi.com/ )
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

#include "../miui_inter.h"
#include "../miui.h"
/*
 *author:dennis 
 *mail:yanhao@xiaomi.com
 *create a menu node that contain back ui, 
 *
 */

struct _menuUnit * menuNode_init(struct _menuUnit *node)
{
    return_null_if_fail(node != NULL)
    node->child = NULL;
    return node;
}

STATUS menuNode_add(struct _menuUnit *parent, struct _menuUnit *child)
{
    return_val_if_fail(parent != NULL, RET_FAIL);
    return_val_if_fail(child != NULL, RET_FAIL);
    return_val_if_fail(parent->get_child_count(parent) < ITEM_COUNT, RET_FAIL);
    if (parent->child == NULL) {
        parent->child = child;
        child->parent = parent;
        return RET_OK;
    }
    struct _menuUnit *temp = parent->child;
    while (temp->nextSilbing != NULL)
    {
        temp = temp->nextSilbing;
    }
    temp->nextSilbing = child;
    child->parent = parent;
    return RET_OK;
}

static STATUS _menuNode_clear(struct _menuUnit *p)
{
    //release tree, post order release
    if (p == NULL)
        return RET_OK;
    _menuNode_clear(p->child);
    _menuNode_clear(p->nextSilbing);
    free(p);
    return RET_OK;
}
STATUS menuNode_delete(struct _menuUnit *parent, struct _menuUnit *child)
{
    return_val_if_fail(parent != NULL, RET_FAIL);
    return_val_if_fail(child != NULL, RET_FAIL);
    return_val_if_fail(parent->child != NULL, RET_FAIL);
    struct _menuUnit *pb = parent->child;
    if (pb == child)
    {
        _menuNode_clear(pb->child);
        parent->child = pb->nextSilbing;
        free(pb);
        return RET_OK;
    }
    struct _menuUnit *p = pb->nextSilbing;
    while(p != NULL)
    {
        if (p == child)
        {
            _menuNode_clear(pb->child);
            pb->nextSilbing = p->nextSilbing;
            free(pb);
            RET_OK;
        }
        else {
            pb = p;
            p = pb->nextSilbing;
        }
    }
    return RET_FAIL;
}
