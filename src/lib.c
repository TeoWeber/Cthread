
#include <stdio.h>
#include <string.h>
#include "../include/support.h"
#include "../include/cthread.h"
#include "../include/cdata.h"

#define STACK_SS_SIZE 64000
#define SUCCESS 0
#define CIDENTIFY_SIZE_ERROR -1
#define EMPTY_QUEUE_ERROR -2
#include <stdio.h>
#include <string.h>
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

int main_thread = 0;
int next_tid_available = 0;
FILA2 ready_queue;
TCB_t* running_queue;

// Contexto do escalonador
ucontext_t schedulerContext;

TCB_t* cmax_prio_pop (PFILA2 pfila) {
    if (FirstFila2(pfila) != SUCCESS) {
        return NULL;
    }
    else {
        TCB_t* tcb = GetAtIteratorFila2(pfila);
        int max_prio = tcb->prio;
        NODE2* max_prio_it = pfila->it;
        while (NextFila2(pfila) != NXTFILA_ENDQUEUE) {
            tcb = GetAtIteratorFila2(pfila);
            if (tcb->prio < max_prio) {
                max_prio = tcb->prio;
                max_prio_it = pfila->it;
            }
        }
        pfila->it = max_prio_it;
        tcb = GetAtIteratorFila2(pfila);
        DeleteAtIteratorFila2(pfila);
        return tcb;
    }
}

int cscheduler () {
    if (running_queue->state == PROCST_EXEC) {
        running_queue->state = PROCST_TERMINO;
        // Liberar threads esperando por essa thread via cjoin
    }
    else {
        running_queue->prio = (int) stopTimer();
        if (running_queue->state == PROCST_APTO) {
            AppendFila2(&ready_queue, &running_queue);
        }
    }

    running_queue = cmax_prio_pop(&ready_queue);
    running_queue->state = PROCST_EXEC;

    startTimer();
    setcontext(&running_queue->context);

    return SUCCESS;
}

int cmain_thread_init () {
    startTimer();
    main_thread = 1;

    // Inicializando o contexto do escalonador:
    getcontext(&schedulerContext);
    schedulerContext.uc_link = 0;
    schedulerContext.uc_stack.ss_sp = (char *) malloc (STACK_SS_SIZE);
    schedulerContext.uc_stack.ss_size = STACK_SS_SIZE;
    makecontext(&schedulerContext, (void (*)(void)) cscheduler , 0);

    // Definindo o tcb da thread main:
	TCB_t* tcb = (TCB_t*)malloc(sizeof(TCB_t));

    if (next_tid_available != 0) {
        return RESERVED_TID_ERROR;
    }

	tcb->tid = next_tid_available;
	next_tid_available++;
	tcb->state = PROCST_EXEC;
	tcb->prio = 0; // VERIFICAR (deve ser 0?)

	getcontext(&(tcb->context));
    // FAZER DEMAIS MODIFICAÇÕES (precisa?) (estão corretas?)
	// tcb->context.uc_link = 0; ou tcb->context.uc_link = &schedulerContext;
	// tcb->context.uc_stack.ss_sp = malloc(STACK_SS_SIZE);
	// tcb->context.uc_stack.ss_size = STACK_SS_SIZE;
	// tcb->context.uc_stack.ss_flags = 0;

    return SUCCESS;
}

int ccreate (void* (*start)(void*), void *arg, int prio) {
    if (!main_thread) {
        cmain_thread_init();
    }

	CreateFila2(&ready_queue);

	TCB_t* tcb = (TCB_t*)malloc(sizeof(TCB_t));

	tcb->tid = next_tid_available;
	next_tid_available++;
	tcb->state = PROCST_APTO;
	tcb->prio = prio;

	getcontext(&(tcb->context));
	tcb->context.uc_link = &schedulerContext;
	tcb->context.uc_stack.ss_sp = malloc(STACK_SS_SIZE);
	tcb->context.uc_stack.ss_size = STACK_SS_SIZE;
	tcb->context.uc_stack.ss_flags = 0;
	makecontext(&(tcb->context), (void (*)(void))start, 1, arg);

	return tcb->tid;
}


int cyield(void) {
    if (!main_thread) {
        cmain_thread_init();
    }

    running_queue->state = PROCST_APTO;
    swapcontext(&current_thread->context, &schedulerContext);

    return SUCCESS;
}


int cjoin(int tid) {
    if (!main_thread) {
        cmain_thread_init();
    }

	return -1;
}

int csem_init(csem_t *sem, int count) {
    if (!main_thread) {
        cmain_thread_init();
    }

    sem = (csem_t*)malloc(sizeof(csem_t));
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

        AppendFila2(sem->fila, &running_queue);
        current_thread->state = PROCST_BLOQ;
        swapcontext(&current_thread->context, &schedulerContext);

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
        tcb->state = PROCST_APTO;
        AppendFila2(&ready_queue, &tcb);
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
