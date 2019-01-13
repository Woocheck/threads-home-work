/*
Zadanie
1. Zaimplementuj stos przy wykorzystaniu tablicy o rozmiarze N zawierające dane typu
unsigned int.
2. Zaimplementuj metody dodawania/usuwania do/z stosu:
int push_front()
• jeżeli stos jest pełny, wówczas zwracana wartość ma wynosić -1
int pop_front()
• jeżeli stos jest pusty, wówczas zwracana wartość ma wynosić -1
3. Instancja tego stosu ma znajdować się w pamięci współdzielonej dla dwóch powiązanych
procesów (producent, konsument).
4. Zidetyfikuj sekcje krytyczne w programie i odpowiednio je zabezpiecz.
5. Jeżeli stos jest pusty, wówczas czytający proces (konsument) ma być uśpiony.
6. Jeżeli stos jest pełny, wówczas zapisujący proces (producent) ma być uśpiony.
schemat 1-producent, 1-konsument, k-zasobów
*/
#include <stdio.h>
#include <stdlib.h>   // EXIT_SUCCESS
#include <sys/mman.h> // PROT_*, MAP_SHARED 
#include <fcntl.h>    // O_* constants
#include <unistd.h>   // ftruncate, close
#include <sys/wait.h> // wait
#include <semaphore.h> // semaphore


#define N 10 //max number elements on stack

typedef struct stack {
    
	unsigned int stack[N];
	int first_out_number;
} stack;

int push_front(stack*, unsigned int);
int pop_front(stack*);


int main(int argc, char *argv[])
{	
	stack * shm_ptr  = NULL;
	int        shm_fd    = shm_open("/my_shm", O_CREAT | O_EXCL | O_RDWR, 0600);
	sem_t    * sem_writer_lock  = sem_open("/my_sem_writer_lock",  O_CREAT | O_EXCL | O_RDWR, 0600, 0); 
	sem_t    * sem_reader_lock = sem_open("/my_sem_reader_lock", O_CREAT | O_EXCL | O_RDWR, 0600, N); //N - max number elements on stack
	int        pid;
	int        status;
	
	// size is zero, resize
	ftruncate(shm_fd, sizeof(stack));
	
	// map a POSIX shared memory object to the calling process's virtual address space 
	shm_ptr = mmap(NULL, 	              // kernel chooses the address at which to create the mapping
				  sizeof(stack),       	  // length of the mapping
				  PROT_READ | PROT_WRITE, // memory protection of the mapping
				  MAP_SHARED,             // update to the mapping is visible to other processes
				  shm_fd, 				  // SHM descriptor from shm_open() 
				  0);
	int i=0;
    int pop_push_result=0;
	shm_ptr->first_out_number=-1;
	
    pid = fork();
	if(pid == -1) {
		exit(EXIT_FAILURE);
	}
	else if(pid == 0) {
		//child - writer

        // make some source data
        unsigned int source[10*N];
        for(int i=0; i<(10*N); ++i) source[i]=i;	
		
        while(1)
        {			
			sem_wait(sem_reader_lock); 
                if(shm_ptr->first_out_number<N-1)
                {
                    pop_push_result=push_front(shm_ptr,source[i]);
                    if(pop_push_result==0) ++i;
                }
			sem_post(sem_writer_lock);
            if(i>=10*N)break;
		}

		sem_close(sem_reader_lock);
		sem_close(sem_writer_lock);
		close(shm_fd);
		exit(EXIT_SUCCESS);
	}
	else {
		//parent - reader

        //make results conainer
        unsigned int results[10*N];
        for(int i=0; i<(10*N); ++i) results[i]=0;
		
		while(1)
        {				
			sem_wait(sem_writer_lock); 
				
                if(shm_ptr->first_out_number>-1)
                {
                    pop_push_result=pop_front(shm_ptr);
                    if(pop_push_result>=0) 
                    {
                        ++i;
                        results[i]=pop_push_result;
                    }
                }
			sem_post(sem_reader_lock);
            if(i>=10*N)break;
		}
		
        printf("All data has been received.\n");
        for(int i=0; i<(10*N); ++i) 
        {
            printf("%d, ",results[i]);
            if(!(i%10)) printf("\n");
        }
		printf("\n");

		wait(&status);
		
		sem_close(sem_reader_lock);
		sem_close(sem_writer_lock);
		close(shm_fd);
		sem_unlink("/my_sem_writer_lock");
		sem_unlink("/my_sem_reader_lock");
		shm_unlink("/my_shm13");
		exit(EXIT_SUCCESS);
	}
}

int push_front(stack* stack, unsigned int element)
{
	if(stack->first_out_number>=N-1)
	{
		printf("Wait! Stack overflow.\n");
		return -1;
	}	
	stack->first_out_number+=1;
	stack->stack[stack->first_out_number]=element;

	return 0;		
}

int pop_front(stack* stack)
{
	if(stack->first_out_number<0)
	{
		printf("We hve some problems with this stack.\n");
		return -1;
	}
	int element = stack->stack[stack->first_out_number];
	stack->first_out_number-=1;

	return element;
}