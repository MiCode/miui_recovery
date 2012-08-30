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
    struct _menuUnit *back = back_ui_init();
    return_null_if_fail(back != NULL);
    node->child = back;
    return node;
}

STATUS menuNode_add(struct _menuUnit *parent, struct _menuUnit *child)
{
    return_val_if_fail(parent != NULL, RET_FAIL);
    return_val_if_fail(child != NULL, RET_FAIL);
    return_val_if_fail(parent->get_child_count(parent) < ITEM_COUNT, RET_FAIL);
    struct _menuUnit *temp = parent->child;
    return_val_if_fail(temp != NULL, RET_FAIL);
    struct _menuUnit *temp_next = temp->nextSilbing;
    if (temp_next == NULL)
    {
        parent->child = child;
        child->parent = parent;
        child->nextSilbing = temp;
        return RET_OK;
    }
    while (temp_next->nextSilbing != NULL)
    {
        temp = temp_next;
        temp_next = temp->nextSilbing;
    }
    temp->nextSilbing = child;
    child->parent = parent;
    child->nextSilbing = temp_next;
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
