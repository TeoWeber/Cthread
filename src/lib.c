
#include <stdio.h>
#include <string.h>
#include "../include/support.h"
#include "../include/cthread.h"
#include "../include/cdata.h"

#define STACK_SS_SIZE 64000
#define SUCCESS 0;
#define CIDENTIFY_SIZE_ERROR -1;
#define EMPTY_QUEUE_ERROR -2;

int last_tid = 0;
FILA2 ready_queue;
TCB_t* running_queue;

int ccreate (void* (*start)(void*), void *arg, int prio) {
	CreateFila2(&ready_queue);

	TCB_t* tcb = (TCB_t*)malloc(sizeof(TCB_t));

	tcb->tid = last_tid;
	last_tid++;
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
	return -1;
}

int cjoin(int tid) {
	return -1;
}

int csem_init(csem_t *sem, int count) {
	return -1;
}

int cwait(csem_t *sem) {
    if ((csem_t->count) > 0) {
        (csem_t->count)--;
        return 0;
    }
    else {
        (csem_t->count)--;
        running_queue->state = PROCST_BLOQ;
        AppendFila2(csem_t->fila, running_queue);
        running_queue = nullptr;
        return SUCCESS;
    }
}

int csignal(csem_t *sem) {
    (csem_t->count)++;
    if (FirstFila2(csem_t->fila) != 0) {
        return EMPTY_QUEUE_ERROR;
    }
    else {
        TCB_t* tcb = GetAtIteratorFila2(csem_t->fila);
        int max_prio = tcb->prio;
        sFilaNode2* max_prio_it = (csem_t->fila)->it;
        while (NextFila2(csem_t->fila) != NXTFILA_ENDQUEUE) {
            tcb = GetAtIteratorFila2(csem_t->fila);
            if (tcb->prio > max_prio) {
                max_prio = tcb->prio;
                max_prio_it = (csem_t->fila)->it;
            }
        }
        (csem_t->fila)->it = max_prio_it;
        TCB_t* tcb = GetAtIteratorFila2(csem_t->fila);
        tcb->state = PROCST_APTO;
        AppendFila2(&ready_queue, &tcb);
        DeleteAtIteratorFila2(csem_t->fila);
        return SUCCESS;
    }
}

int cidentify (char *name, int size) {
	char* grupo = "Astelio Jose Weber (283864)\nFrederico Schwartzhaupt (304244)\nJulia Violato (290185)";

	if (strlen(grupo) > size) {
		return CIDENTIFY_SIZE_ERROR;
	}
	else {
		strcpy(name, grupo);
		return SUCCESS;
	}
}


