#include <threading.h>

void t_init()
{
    //Initialize all contexts to INVALID
    for(int i = 0; i < NUM_CTX; i++){
        contexts[i].state = INVALID;
    }
    //Contexts[0] as main thread. set current_context_idx = 0
    current_context_idx = 0;
    contexts[0].state = VALID;
}

int32_t t_create(fptr foo, int32_t arg1, int32_t arg2)
{
    // find free slot
    uint8_t slot = NUM_CTX;
    for (uint8_t i = 0; i < NUM_CTX; i++) {
        if (contexts[i].state == INVALID) {
        slot = i;
        break;
        }
    }

    if (slot == NUM_CTX) {
        return 1; // No free slots
    }
    getcontext(&contexts[slot].context);
    //stack
    contexts[slot].context.uc_stack.ss_sp = malloc(STK_SZ);
    contexts[slot].context.uc_stack.ss_size = STK_SZ;
    contexts[slot].context.uc_stack.ss_flags = 0;
    contexts[slot].context.uc_link = NULL;
    
    // make frame for foo(arg1, arg2)
    makecontext(&contexts[slot].context, (ctx_ptr)foo, 2, arg1, arg2);
    
    //mark valid
    contexts[slot].state = VALID;
    
    return 0;
}

int32_t t_yield()
{
    // Find next valid context
    uint8_t next = NUM_CTX;
    for (uint8_t i = 0; i < NUM_CTX; i++) {
        int idx = (current_context_idx + 1 + i) % NUM_CTX;
        if (contexts[idx].state == VALID) {
            next = (uint8_t)idx;
            break;
        }
    }
    
    if (next == NUM_CTX) {
        return -1; //failed
    }
    
    // Save where we are now
    uint8_t old_idx = current_context_idx;
    current_context_idx = next;
    
    // Switch context. Save old context, go to new one
    swapcontext(&contexts[old_idx].context, &contexts[next].context);
    
    // When we return here later, count valid contexts
    int count = 0;
    for (int i = 0; i < NUM_CTX; i++) {
        if (i != current_context_idx && contexts[i].state == VALID) {
            count++;
        }
    }
    
    return count;
}

void t_finish()
{
    //Free the stack we allocated in t_create() by worker takss
    free(contexts[current_context_idx].context.uc_stack.ss_sp);
    
    // Mark this context as invalid so it won't be scheduled again
    contexts[current_context_idx].state = INVALID;
    
    // fidn another valid context to switch to
    uint8_t next = NUM_CTX;
    for (uint8_t i = 0; i < NUM_CTX; i++) {
        if (contexts[i].state == VALID) {
            next = i;
            break;
        }
    }
    
    //Switch to the next valid context
    if (next != NUM_CTX) {
        current_context_idx = next;
        setcontext(&contexts[next].context);
        // setcontext() never returns if successful
    }
    
    // If we get here, something went wrong (no valid contexts)
    
}
