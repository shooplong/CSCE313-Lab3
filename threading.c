#include <threading.h>

void t_init()
{
        // TODO
        // initialize various data structures that your library uses, such as contexts and current_context_idx

        // contexts is already defined/declared inside of threading_data.c, i think they are just declared in threading.h
        // so now just initialize each state in contexts to be INVALID meaning we still have to save for each one, its like pre pre-screen
        // contexts was defined with enough space for NUM_CTX amount of elements
        // also contexts is a struct of worker_context
        for (int i = 0; i < NUM_CTX; i++) {
                contexts[i].state = INVALID;
        }

        // current_context_idx keeps track of which context is active right now, so none of the 0 to NUM_CTX - 1 contexts are active rn.
        // initialize current_context_idx to an random bad value b/c we haven't done jack shi yet
        current_context_idx = NUM_CTX; // initialize to NUM_CTX b/c there are NUM_CTX - 1 valid contexts, so NUM_CTX would be bad

}

int32_t t_create(fptr foo, int32_t arg1, int32_t arg2)
{
        // TODO
        // find first invalid state so we can start using it

        // first need to find first invalid state index in our contexts[] array
        int firstInvalid = NUM_CTX;
        for (int i = 0; i < NUM_CTX; i++){
                if (contexts[i].state == INVALID){
                        firstInvalid = i;
                        break;
                }
        }

        // need to copy current process context, just use getcontext()
        getcontext(&contexts[firstInvalid].context);

        // modify it by allocating memory on the stack
        // build a "stack frame" on the stack so when we restore the process state
        // from this context, the process will start executing as if the function had been called
        contexts[firstInvalid].context.uc_stack.ss_sp = (char*) malloc(STK_SZ); // STK_SZ is set to 4096 in threading.h
        contexts[firstInvalid].context.uc_stack.ss_size = STK_SZ;
        contexts[firstInvalid].context.uc_stack.ss_flags = 0; // set to 0 b/c the child isn't required to handle any signals
        contexts[firstInvalid].context.uc_link = NULL; // set to NULL to indicate that once the function finishes
        // execution, the worker can simply terminate. Can optionally set to another ucontext_t obj to resume that

        // call makecontext now to actually prep context for future execution of foo
        makecontext(&contexts[firstInvalid].context, (ctx_ptr)foo, 2, arg1, arg2);

        // set that firstInvalid state to valid now :D
        contexts[firstInvalid].state = VALID; // so now its ready to be used, AKA VALID

        return 0; // if we get to this point, then success! return 0 per threading.h instructions
        
}

int32_t t_yield()
{
        // TODO
}

void t_finish()
{
        // TODO
}
