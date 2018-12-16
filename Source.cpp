#include <stdio.h>
#include <stdlib.h>

#define STACK_MAX_SIZE 256
#define GC_START_MAX_OBJECTS 8

enum object_type
{
	INT,
	TWIN
};

struct object
{
	object_type Type;
	unsigned char Marked;

	object* Next;

	union
	{
		int Value;

		struct
		{
			object *Head;
			object *Tail;
		};
	};
};

struct vm
{
	object* Stack[STACK_MAX_SIZE];
	int StackSize;

	int NumObjects;
	int MaxObjects;

	object* FirstObject;
};

vm* NewVM()
{
	vm* VM = (vm*)malloc(sizeof(vm));
	VM->StackSize = 0;
	VM->FirstObject = 0;

	VM->NumObjects = 0;
	VM->MaxObjects = GC_START_MAX_OBJECTS;

	return(VM);
}

void Push(vm* VM, object *Value)
{
	VM->Stack[VM->StackSize++] = Value;
}

object* Pop(vm* VM)
{
	object* Result = VM->Stack[--VM->StackSize];

	return(Result);
}

void Mark(object* Object)
{
	if (!Object->Marked)
	{
		Object->Marked = 1;

		if (Object->Type == TWIN)
		{
			Mark(Object->Head);
			Mark(Object->Tail);
		}
	}
}

void MarkAll(vm* VM)
{
	for (int ObjectIndex = 0; ObjectIndex < VM->StackSize; ObjectIndex++)
	{
		Mark(VM->Stack[ObjectIndex]);
	}
}

void MarkSweep(vm* VM)
{
	object** Object = &VM->FirstObject;
	while (*Object)
	{
		if (!(*Object)->Marked)
		{
			object* Unreached = *Object;
			*Object = Unreached->Next;
			free(Unreached);

			VM->NumObjects--;
		}
		else
		{
			(*Object)->Marked = 0;
			Object = &(*Object)->Next;
		}
	}
}

void GC(vm* VM)
{
	int NumObjects = VM->NumObjects;

	MarkAll(VM);
	MarkSweep(VM);

	VM->MaxObjects = VM->NumObjects * 2;

	printf("Collected %d objects, %d left.\n", NumObjects - VM->NumObjects, VM->NumObjects);
}

object* NewObject(vm* VM, object_type Type)
{
	if (VM->NumObjects == VM->MaxObjects)
	{
		GC(VM);
	}

	object* Object = (object*)malloc(sizeof(object));
	Object->Type = Type;
	Object->Next = VM->FirstObject;
	VM->FirstObject = Object;
	Object->Marked = 0;

	VM->NumObjects++;

	return(Object);
}

void PushInt(vm* VM, int IntValue)
{
	object* Object = NewObject(VM, INT);
	Object->Value = IntValue;

	Push(VM, Object);
}

object* PushTwin(vm* VM)
{
	object* Object = NewObject(VM, TWIN);
	Object->Tail = Pop(VM);
	Object->Head = Pop(VM);

	Push(VM, Object);
	return(Object);
}

void FreeVM(vm* VM)	
{
	VM->StackSize = 0;
	GC(VM);
	free(VM);
}

void ObjectPrint(object* Object)
{
	switch (Object->Type)
	{
		case INT:
		{
			printf("%d", Object->Value);
		} break;

		case TWIN:
		{
			printf("(");
			ObjectPrint(Object->Head);
			printf(", ");
			ObjectPrint(Object->Tail);
			printf(")");
		} break;
	}
}

void FirstTest()
{
	printf("1: Objects on the stack are preserved.\n");
	vm* VM = NewVM();
	PushInt(VM, 0);
	PushInt(VM, 1);

	GC(VM);
	FreeVM(VM);
}

void SecondTest()
{
	printf("2: Unreached objects are collected.\n");
	vm* VM = NewVM();
	PushInt(VM, 0);
	PushInt(VM, 1);
	Pop(VM);
	Pop(VM);

	GC(VM);
	FreeVM(VM);
}

void ThirdTest()
{
	printf("3: Reach the nested objects.\n");
	vm* VM = NewVM();
	PushInt(VM, 0);
	PushInt(VM, 1);
	PushTwin(VM);
	PushInt(VM, 2);
	PushInt(VM, 3);
	PushTwin(VM);
	PushTwin(VM);

	GC(VM);
	FreeVM(VM);
}

void CyclesTest()
{
	printf("4: Cycles.\n");
	vm* VM = NewVM();
	PushInt(VM, 0);
	PushInt(VM, 1);
	object* A = PushTwin(VM);
	PushInt(VM, 2);
	PushInt(VM, 3);
	object* B = PushTwin(VM);

	A->Tail = B;
	B->Tail = A;

	GC(VM);
	FreeVM(VM);
}

int main(void)
{
	FirstTest();
	SecondTest();
	ThirdTest();
	CyclesTest();

	return(0);
}