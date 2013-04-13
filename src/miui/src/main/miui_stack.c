#include "../miui_inter.h"
#include "../miui.h"

//iniital miuiStack
static struct _miuiStack miui_stack;
static struct _miuiStack* const s = &miui_stack;
STATUS miuiStack_init()
{
    return_val_if_fail(s != NULL, RET_FAIL);
    s->top = -1;
    return RET_OK;
}
//judge empty
STATUS miuiStack_isEmpty()
{
    return_val_if_fail(s != NULL, RET_FAIL);
    return s->top == -1;
}

STATUS miuiStack_isFull()
{
    return_val_if_fail(s != NULL, RET_FAIL);
    return s->top == (MAX_STACK_SIZE - 1);
}

STATUS miuiStack_push(dataType d)
{
    return_val_if_fail(miuiStack_isFull() == 0, RET_FAIL);
    s->data[++s->top] = d;
    return RET_OK;
}

dataType miuiStack_pop()
{
    return_null_if_fail(miuiStack_isEmpty() == 0);
    return (s->data[s->top--]);
}

dataType miuiStack_getTop()
{
    return_null_if_fail(miuiStack_isEmpty() == 0);
    return (s->data[s->top]);
}
STATUS miuiStack_disp()
{
    return_val_if_fail(miuiStack_isEmpty() == 0, RET_FAIL);
    int count = s->top;
    miui_printf("the element count is %d\n", count);
    return RET_OK;
}

