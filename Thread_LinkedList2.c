#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>  // EXIT_SUCCESS
#include <pthread.h> // pthread


#define MAX_ELEMS 10
#define PRODUCENTS_NUMBER 5
#define CONSUMER_NUMBER 8

pthread_mutex_t my_mutex;
pthread_cond_t buff_cond_full, buff_cond_empty;

typedef struct node
{
    int value;
    struct node* next;
} node;

typedef struct Lists
{
    int elements_number;
    node* head;
    node* tail;
} List;

int push_back(List** list, int new_value) 
{ 
   
    if((*list) == NULL)
    {
        printf("No solution available, list is empty.\n");
        return -1;
    }
    else if((*list)->elements_number >= MAX_ELEMS) 
    {
        printf("There is no space in fifo FIFO. %d \n", (*list)->elements_number);
        return -1;
    }
    else
    {   
        struct node* new_node = (struct node*)malloc(sizeof(struct node)); 
        new_node->value= new_value; 
        new_node->next=NULL;

        if((*list)->head==NULL) 
            (*list)->head=(*list)->tail=new_node;
        else
        {
            (*list)->tail->next=new_node;
            (*list)->tail=new_node;
        }
        (*list)->elements_number+=1;
        return 0;
        
    }
}

int pop_front(List** list)
{
    if((*list) == NULL)
    {
        printf("No solution available, exiting program.\n");
        exit(-1);
    }
    else if((*list)->head==(*list)->tail)
    {
        int pop_element=(*list)->head->value;
        (*list)->head=(*list)->tail=NULL;
        free((*list)->head);
        free((*list)->tail);
        (*list)->elements_number=0;
        return pop_element;
    }
    else
    {
        struct node* old_head=NULL;
        int pop_element=(*list)->head->value;
        old_head=(*list)->head;
        (*list)->head=(*list)->head->next;
        free(old_head);
        (*list)->elements_number-=1;
        return pop_element;
    }
}
     
void print_list(List** list)
{
    node* list_element = (*list)->head; 
    while(list_element != NULL) {
        printf("%d ", list_element->value);
        list_element = list_element->next;
    }
    printf("\n");
}

void* fun_write(void *arg)
{
	int index = 0;
    printf("In write: Elements number %d \n", ((List*)arg)->elements_number);//linijka do sprawdzania przekazanego argumentu
	while(index<100)
    {
	    pthread_mutex_lock(&my_mutex);
	    // jezeli nie ma miejsca do zapisu czekaj
	    if(((struct Lists*)arg)->elements_number >= MAX_ELEMS)
	            pthread_cond_wait(&buff_cond_empty, &my_mutex);
        push_back(((struct Lists**)arg), index);

	    // oznacz, ze nie ma juz miejsca do zapisu i poinformuj o tym watek odczytujacy
	    if(((struct Lists*)arg)->elements_number >= MAX_ELEMS)
                    pthread_cond_signal(&buff_cond_full);
	    pthread_mutex_unlock(&my_mutex);
    }
	pthread_exit(NULL);
}

void* fun_read(void* arg)
{
    int element = 0;
    printf("In read: Elements number %d \n", ((List*)arg)->elements_number); //linijka do sprawdzania przekazanego argumentu
	while(1)
    {
	    pthread_mutex_lock(&my_mutex);
	    // jezeli nie ma miejsca do zapisu czekaj
	    if(((struct Lists*)arg)->elements_number <= 0)
	            pthread_cond_wait(&buff_cond_full, &my_mutex);
        element = pop_front(((struct Lists**)arg));
        printf("Element value: %d .\n", element);
	    // oznacz, ze nie ma juz miejsca do zapisu i poinformuj o tym watek odczytujacy
	    if(((struct Lists*)arg)->elements_number <= 0)
                    pthread_cond_signal(&buff_cond_empty);
	    pthread_mutex_unlock(&my_mutex);
    }
	pthread_exit(NULL);
	
}

int main()
{    
    //FIFO initialization
    struct Lists* list = (struct Lists*)malloc(sizeof(struct Lists));    
    list->elements_number = 0;
    list->head=NULL;
    list->tail=NULL;
    //Threads initialisation
    pthread_t thread_producent[PRODUCENTS_NUMBER];
    pthread_t thread_consumer[CONSUMER_NUMBER];
	
    // inicjalizacja mutexu
	pthread_mutex_init(&my_mutex, NULL);

	// inicjalizacja zmiennej warunku
	pthread_cond_init(&buff_cond_empty, NULL);
	pthread_cond_init(&buff_cond_full,  NULL);
    
	for(int i = 0 ; i < PRODUCENTS_NUMBER ; ++i)
		pthread_create(&thread_producent[i], NULL, &fun_write, &list);
    for(int i = 0 ; i < CONSUMER_NUMBER ; i++)
		pthread_create(&thread_consumer[i], NULL, &fun_read, &list);

	for(int i = 0 ; i < PRODUCENTS_NUMBER ; i++)
		pthread_join(thread_producent[i], NULL);
    for(int i = 0 ; i < CONSUMER_NUMBER ; i++)
		pthread_join(thread_consumer[i], NULL);
    
    // zniszczenie mutexu
	pthread_mutex_destroy(&my_mutex);

	// zniszczenie zmiennej warunku
	pthread_cond_destroy(&buff_cond_empty);
	pthread_cond_destroy(&buff_cond_full);
    
    exit(EXIT_SUCCESS);
}
