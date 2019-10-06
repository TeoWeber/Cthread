
#include <stdio.h>
#include <string.h>
#include "../include/support.h"
#include "../include/cthread.h"
#include "../include/cdata.h"

#define STACK_SS_SIZE 64000
#define SUCCESS 0;
#define CIDENTIFY_SIZE_ERROR -1;
#define EMPTY_QUEUE_ERROR -2;
#define CREATE_QUEUE_ERROR -3;

bool main_thread = false;
int next_tid_available = 0;
FILA2 ready_queue;
TCB_t* running_queue;

TCB_t* cmax_prio_pop (PFILA2 pfila) {
    if (FirstFila2(pfila) != SUCCESS) {
        return nullptr;
    }
    else {
        TCB_t* tcb = GetAtIteratorFila2(pfila);
        int max_prio = tcb->prio;
        sFilaNode2* max_prio_it = pfila->it;
        while (NextFila2(pfila) != NXTFILA_ENDQUEUE) {
            tcb = GetAtIteratorFila2(pfila);
            if (tcb->prio > max_prio) {
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
    return SUCCESS;
}

int cmain_thread_init () {
    main_thread = true;
    return SUCESS;
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

	// referência: https://stackoverflow.com/questions/21468529/context-switching-ucontext-t-and-makecontext (revisar)
	getcontext(&(tcb->context));
	tcb->context.uc_link = 0;
	tcb->context.uc_stack.ss_sp = malloc(STACK_SS_SIZE);
	tcb->context.uc_stack.ss_size = STACK_SS_SIZE;
	tcb->context.uc_stack.ss_flags = 0;
	makecontext(&(tcb->context), (void (*)(void))start, 1, arg);

	// verificação se a criação da thread foi bem sucedida

	AppendFila2(&ready_queue, &tcb);

	return tcb->tid;
}

int cyield(void) {
    if (!main_thread) {
        cmain_thread_init();
    }
    
    TCB_t* tcb = running_queue;
    tcb->state = PROCST_APTO;

    running_queue = NULL;

    AppendFila2(&ready_queue, &tcb);

    if(cscheduler() == SUCCESS) {
        return SUCCESS;
    }

    else {
        return -1;
    }
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

    if ((csem_t->count) > 0) {
        (csem_t->count)--;
        return 0;
    }
    else {
        (csem_t->count)--;
        running_queue->state = PROCST_BLOQ;
        AppendFila2(csem_t->fila, running_queue);
        running_queue = nullptr;
        cscheduler();
        return SUCCESS;
    }
}

int csignal(csem_t *sem) {
    if (!main_thread) {
        cmain_thread_init();
    }

    (csem_t->count)++;
    TCB_t* tcb = cmax_prio_pop(csem_t->fila);
    if (tcb == nullptr) {
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

