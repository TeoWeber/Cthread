
#include <stdio.h>
#include <string.h>
#include "../include/support.h"
#include "../include/cthread.h"
#include "../include/cdata.h"

#define STACK_SS_SIZE 64000
#define SUCCESS 0;
#define CIDENTIFY_SIZE_ERROR -1;

int last_tid = 0;
FILA2 ready_queue;

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
	return -1;
}

int csignal(csem_t *sem) {
	return -1;
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


