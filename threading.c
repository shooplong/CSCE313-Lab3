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
        volatile int firstInvalid = NUM_CTX;
        for (volatile int i = 0; i < NUM_CTX; i++){
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
        // This function is called by a worker to indicate that it can relinquish the control over to other workers
        // This function may or may not return in the caller
        // returns: This function returns the number of contexts (apart from the caller) 
        // which are in the VALID state if it is successful, otherwise it returns -1

        // first save the current worker's context
        getcontext(&contexts[current_context_idx].context); // updates the current context (whose index is storeed in current_context_idx)
        // -- by taking a fresh snapshot using getcontext

        // search for VALID context and use swapcontext to switch to it
        // swapcontext(ucontext_t* from, ucontext_t* to);
        for (uint8_t i = 1; i <= NUM_CTX; i++){
                uint8_t nextValid = (current_context_idx + i) % NUM_CTX; // mod b/c what if we were at context 12 or sum and we need to wrap around to the beginning 
                if (contexts[nextValid].state == VALID) {
                        int fromContextIdx = current_context_idx; // this is the one we are switching from
                        current_context_idx = nextValid; // set the context we are switching TO to the current, b/c current is now set to that next valid one
                        swapcontext(&contexts[fromContextIdx].context, &contexts[current_context_idx].context);
                        break; // can break now since we swapped
                }
        }
        // return number of VALID states if successful, if not return -1
        int32_t numValid = 0;
        for (int i = 0; i < NUM_CTX; i++){
                if (contexts[i].state == VALID){
                        numValid++;
                }
        }

        if (numValid > 0) {
                return (numValid - 1); // return numValid - 1 b/c threading.h says to exclude the caller
        } else {
                return -1;
        }
}

void t_finish()
{
        // TODO
        // this is called by the worker context to indicate that it has completed its work
        // after this function is called, worker's context is deleted and the worker is never scheduled again. set state to DONE

        // mark current worker to done
        contexts[current_context_idx].state = DONE; // this is kinda sad :(

        // delete the memory off stack
        free(contexts[current_context_idx].context.uc_stack.ss_sp); // we prepped this earlier.
        contexts[current_context_idx].context.uc_stack.ss_sp = NULL;

        // yield control to next worker
        t_yield();
}
