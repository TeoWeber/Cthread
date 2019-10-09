#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ucontext.h>
#include "../include/support.h"
#include "../include/cthread.h"
#include "../include/cdata.h"

#define STACK_SS_SIZE 64000

#define SUCCESS 0
#define CIDENTIFY_SIZE_ERROR -1
#define EMPTY_QUEUE_ERROR -2
#define CREATE_QUEUE_ERROR -3
#define RESERVED_TID_ERROR -4
#define MALLOC_ERROR -5
#define THREAD_NOT_FOUND -6
#define THREAD_ALREADY_BLOCKING -7
#define NOT_IMPLEMENTED_FLAG -1000

#define IN_READY_QUEUE 1
#define IN_BLOCKED_QUEUE 2

int main_thread = 0;
int next_tid_available = 0;
FILA2 ready_queue;
FILA2 blocked_queue;
TCB_t* running_queue;

// Contexto do escalonador
ucontext_t schedulerContext;

int cscheduler();

TCB_t* cmax_prio_pop (PFILA2 pfila) {
    if (FirstFila2(pfila) != SUCCESS) {
        return NULL;
    }
    else {
        TCB_t* tcb = (TCB_t*)GetAtIteratorFila2(pfila);
        unsigned int max_prio = tcb->prio;
        unsigned int tcb_prio;
        NODE2* max_prio_it = pfila->it;
        while (NextFila2(pfila) != NXTFILA_ENDQUEUE) {
            tcb = (TCB_t*)GetAtIteratorFila2(pfila);
            tcb_prio = tcb->prio; // Cast especial de int com sinal para int sem sinal.
            if (tcb_prio < max_prio) {
                max_prio = tcb_prio;
                max_prio_it = pfila->it;
            }
        }
        pfila->it = max_prio_it;
        tcb = (TCB_t*)GetAtIteratorFila2(pfila);
        DeleteAtIteratorFila2(pfila);
        return tcb;
    }
}

int cmain_thread_init () {
    startTimer();
    main_thread = 1;

    // Inicializa a fila de aptos e bloqueados
    CreateFila2(&ready_queue);
    CreateFila2(&blocked_queue);

    // Inicializando o contexto do escalonador:
    getcontext(&schedulerContext);
    schedulerContext.uc_link = 0;
    if ((schedulerContext.uc_stack.ss_sp = (char *) malloc (STACK_SS_SIZE)) == NULL) {
        return MALLOC_ERROR;
    }
    schedulerContext.uc_stack.ss_size = STACK_SS_SIZE;
    makecontext(&schedulerContext, (void (*)(void)) cscheduler , 0);

    // Definindo o tcb da thread main:
	TCB_t* tcb;
    if ((tcb = (TCB_t*)malloc(sizeof(TCB_t))) == NULL) {
        return MALLOC_ERROR;
    }

    if (next_tid_available != 0) {
        return RESERVED_TID_ERROR;
    }

	tcb->tid = next_tid_available;
	next_tid_available++;
	tcb->state = PROCST_EXEC;
	tcb->prio = 0; 
	tcb->d_tid = -1;
	
	getcontext(&(tcb->context));
    
	running_queue = tcb;

    return SUCCESS;
}

int cfind_thread(int tid) {
    if (running_queue->tid == tid) {
        return THREAD_NOT_FOUND; // Cjoin não deve poder bloquear ele mesmo.
    }
    else {
        TCB_t* tcb;
        PFILA2 pfila;
        
        pfila = &ready_queue;
        if (FirstFila2(pfila) == SUCCESS) {
            do {
                tcb = (TCB_t*)GetAtIteratorFila2(pfila);
                if (tcb->tid == tid) {
                    return IN_READY_QUEUE;
                }
            } while (NextFila2(pfila) != NXTFILA_ENDQUEUE);
        }

        pfila = &blocked_queue;
        if (FirstFila2(pfila) == SUCCESS) {
            do {
                tcb = (TCB_t*)GetAtIteratorFila2(pfila);
                if (tcb->tid == tid) {
                    return IN_BLOCKED_QUEUE;
                }
            } while (NextFila2(pfila) != NXTFILA_ENDQUEUE);

        return THREAD_NOT_FOUND;
    	}
    }
    // procura se a thread com o tid dado existe na ready_queue, running_queue ou blocked_queue
    return THREAD_NOT_FOUND;
}

TCB_t* cpop_thread(PFILA2 pfila, int tid) {
    TCB_t* tcb;

    if (FirstFila2(pfila) != SUCCESS) {
        return NULL;
    }
    else {
        do {
            tcb = (TCB_t*)GetAtIteratorFila2(pfila);
            if (tcb->tid == tid) {
                DeleteAtIteratorFila2(pfila);
                return tcb;
            }
        } while (NextFila2(pfila) != NXTFILA_ENDQUEUE);
        return NULL;
    }
}

int cscheduler () {
    TCB_t* tcb;
    TCB_t* unlockedThread;
    tcb = running_queue;

    if (tcb->state == PROCST_EXEC) {
        stopTimer();
        tcb->state = PROCST_TERMINO;

        if (tcb->d_tid != -1) { // Libera a thread bloqueada em Cjoin(), se existir
            unlockedThread = cpop_thread (&blocked_queue, tcb->d_tid);
            unlockedThread->state = PROCST_APTO;
            AppendFila2(&ready_queue, unlockedThread);
        }
    }
    else {
        tcb->prio = (int) stopTimer();
        if (tcb->state == PROCST_APTO) {
            AppendFila2(&ready_queue, tcb);
        }
        else if (tcb->state == PROCST_BLOQ) {
            AppendFila2(&blocked_queue, tcb);
        }
    }

    running_queue = cmax_prio_pop(&ready_queue);
    running_queue->state = PROCST_EXEC;

    startTimer();
    setcontext(&running_queue->context);

    return SUCCESS;
}

int ccreate (void* (*start)(void*), void *arg, int prio) {   
    if (!main_thread) {
        cmain_thread_init();
    }
	
    TCB_t* tcb;
    if ((tcb = (TCB_t*)malloc(sizeof(TCB_t))) == NULL) {
        return MALLOC_ERROR;
    }

    tcb->tid = next_tid_available;
    next_tid_available++;
    tcb->state = PROCST_APTO;
    tcb->prio = prio;
    tcb->d_tid = -1;

    getcontext(&(tcb->context));
    tcb->context.uc_link = &schedulerContext;
    if ((tcb->context.uc_stack.ss_sp = malloc(STACK_SS_SIZE)) == NULL) {
        return MALLOC_ERROR;
    }
    tcb->context.uc_stack.ss_size = STACK_SS_SIZE;
    tcb->context.uc_stack.ss_flags = 0;
    makecontext(&(tcb->context), start, 1, arg);

    AppendFila2(&ready_queue, tcb);

    return tcb->tid;
}


int cyield(void) {
    if (!main_thread) {
        cmain_thread_init();
    }

    TCB_t* tcb;
    tcb = running_queue;

    tcb->state = PROCST_APTO;
    swapcontext(&tcb->context, &schedulerContext);

    return SUCCESS;
}


int cjoin(int tid) {
    TCB_t* candidate_thread;
    TCB_t* tcb_aux;
    int filaIndicator;
    PFILA2 filaRef;

    if (!main_thread) {
        cmain_thread_init();
    }

    filaIndicator = cfind_thread(tid);      // Informação com a fila da thread do request

    if(filaIndicator == THREAD_NOT_FOUND) { // cfind_thread() retornou erro
	return THREAD_NOT_FOUND;
    }
    if (filaIndicator == IN_READY_QUEUE) {  // cfind_thread() retornou que a thread esta na ready_queue
        filaRef = &ready_queue;
    }
    else {                                  // cfind_thread() retornou que a thread esta na blocked_queue
        filaRef = &blocked_queue;
    }
    
    candidate_thread = cpop_thread(filaRef, tid);   // Pop da thread na sua respectiva lista

    if (candidate_thread->d_tid != -1) {            // Se a thread já esta bloqueando, retorna THREAD_ALREADY_BLOCKING
        AppendFila2(filaRef, candidate_thread);
        return THREAD_ALREADY_BLOCKING;
    }

    candidate_thread->d_tid = running_queue->tid;   // Informamos que a thread do request agora esta bloqueando uma thread
    running_queue->state = PROCST_BLOQ;             // running_queue fica bloqueada

    AppendFila2(filaRef, candidate_thread);         // Devolvemos a thread do request para sua fila
    tcb_aux = running_queue; // Acho que precisa (??)
    swapcontext(&tcb_aux->context, &schedulerContext);

    return SUCCESS;
}



int csem_init(csem_t *sem, int count) {
    if (!main_thread) {
        cmain_thread_init();
    }

    if ((sem = (csem_t*)malloc(sizeof(csem_t))) == NULL) {
        return MALLOC_ERROR;
    }
    sem->count = count;
    if (CreateFila2(sem->fila) == SUCCESS) {
        return SUCCESS;
    }
    else {
        return CREATE_QUEUE_ERROR;
    }
}

int cwait(csem_t *sem) {
    if (!main_thread) {
        cmain_thread_init();
    }

    if (sem->count > 0) {
        sem->count--;
        return 0;
    }
    else {
        sem->count--;

        TCB_t* tcb;
        tcb = running_queue;
        AppendFila2(sem->fila, tcb);
        tcb->state = PROCST_BLOQ;
        swapcontext(&tcb->context, &schedulerContext);

        return SUCCESS;
    }
}

int csignal(csem_t *sem) {
    if (!main_thread) {
        cmain_thread_init();
    }

    sem->count++;
    TCB_t* tcb = cmax_prio_pop(sem->fila);
    if (tcb == NULL) {
        return EMPTY_QUEUE_ERROR;
    }
    else {
        cpop_thread(&blocked_queue, tcb->tid);
        tcb->state = PROCST_APTO;
        AppendFila2(&ready_queue, tcb);
        return SUCCESS;
    }
}

int cidentify (char *name, int size) {
    if (!main_thread) {
        cmain_thread_init();
    }

	char* grupo = "Astelio Jose Weber (283864)\nFrederico Schwartzhaupt (304244)\nJulia Violato (290185)";

	if (strlen(grupo) > size) {
		return CIDENTIFY_SIZE_ERROR;
	}
	else {
		strcpy(name, grupo);
		return SUCCESS;
	}
}
