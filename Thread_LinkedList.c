#include  <stdio.h>
//#include <unistd.h>
#include <stdlib.h>  // EXIT_SUCCESS
#include <pthread.h> // pthread
 
#define MAX_ELEMS 5
#define PRODUCENTS_NUMBER 5
#define CONSUMER_NUMBER 8

pthread_mutex_t my_mutex;
pthread_cond_t buff_cond_full, buff_cond_empty;

struct node
{
  int value;
  struct node* next;
};
 
 
struct List
{
  int elements_number;
  struct node* head;
  struct node* tail;
};

int push_back( struct List*, const int);
int pop_front( struct List*);
 
struct List* list_new(void);
struct List* list_free( struct List* );
 
void list_print( const struct List* );
void list_print_element(const struct node* );

void* fun_write(void *);
void* fun_read(void* );

int main(void)
{
  struct List*  list = NULL;
  list = list_new();

  //Threads initialisation
  pthread_t thread_producent[PRODUCENTS_NUMBER];
  pthread_t thread_consumer[CONSUMER_NUMBER];

  // mutex initialization
  pthread_mutex_init(&my_mutex, NULL);

  list->elements_number=0;
    
  pthread_cond_init(&buff_cond_empty, NULL);
  pthread_cond_init(&buff_cond_full,  NULL);
  printf("Elements number %d \n", list->elements_number); //linijka do sprawdzania przekazanego argumentu
  for(int i = 0 ; i < PRODUCENTS_NUMBER ; ++i)
	  pthread_create(&thread_producent[i], NULL, &fun_write, list);
  for(int i = 0 ; i < CONSUMER_NUMBER ; i++)
	  pthread_create(&thread_consumer[i], NULL, &fun_read, list);
  for(int i = 0 ; i < PRODUCENTS_NUMBER ; i++)
	  pthread_join(thread_producent[i], NULL);
  for(int i = 0 ; i < CONSUMER_NUMBER ; i++)
	  pthread_join(thread_consumer[i], NULL);
  
  pthread_mutex_destroy(&my_mutex);
  
  // zniszczenie zmiennej warunku
  pthread_cond_destroy(&buff_cond_empty);
  pthread_cond_destroy(&buff_cond_full);
  
  exit(EXIT_SUCCESS);
}

int push_back(struct List* list, const int i)
{
    
  struct node* new_element = (struct node*)malloc( sizeof(struct node));
 
  if( NULL == new_element )
    {
      fprintf(stderr, "malloc failed");
      return -2; 
    }
    
  new_element->value = i;
  new_element->next = NULL;
 
  if( NULL == list )
  {
    printf("Queue not initialized\n");
    free(new_element);
    return -2;
  }
  else if( NULL == list->head && NULL == list->tail )
  {
    list->head = list->tail = new_element;
    list->elements_number+=1;
    return 0;
  }
  else if( NULL == list->head || NULL == list->tail )
  {
    fprintf(stderr, "New element prepairing failed.");
    free(new_element);
    return -2;
  }
  else if(list->elements_number>=MAX_ELEMS)
  {
    return -1;
  }
  else
  {
    list->tail->next = new_element;
    list->tail = new_element;
    list->elements_number+=1;
  }
  return 0;
}
 
int pop_front(struct List* list )
{
  struct node* temporary_head = NULL;
  struct node* temporary_next = NULL;
 
  if( NULL == list )
    {
      printf("List is empty\n");
      return -1;
    }
  else if( NULL == list->head && NULL == list->tail )
    {
      printf("List is empty!\n");
      list->elements_number=0;
      return -1;
    }
  else if( NULL == list->head || NULL == list->tail )
    {
      printf("The head or tail is empty while other is not. \n");
      return -1;
    }
    
    else
    {
      int returned_value = list->head->value;
      temporary_head = list->head;
      temporary_next = temporary_head->next;
      free(temporary_head);
      list->head = temporary_next;
      if( NULL == list->head )  list->tail = list->head; 
      list->elements_number-=1;  
 
      return returned_value;
    }
}
   
struct List* list_free( struct List* list )
{
  while( list->head )
    {
      pop_front(list);
    }
 
  return list;
}
 
struct List* list_new(void)
{
  struct List* list = (struct List*) malloc( sizeof(struct List));
 
  if( NULL == list )
    {
      printf("malloc failed\n");
    }
  list->head = list->tail = NULL;
   
  return list;
}
 
 
void list_print( const struct List* list )
{
  struct node* element = NULL;
 
  if( list )
  {
    for( element = list->head; element; element = element->next )
    {
      list_print_element(element);
    }
  }
  printf("\n");
}
 
 
void list_print_element(const struct node* element )
{
  if( element ) 
    {
      printf("Num = %d\n", element->value);
    }
  else
    {
      printf("Can't print NULL.\n");
    }
}

 void* fun_write(void *arg)
{
    struct List* t = ((struct List*)arg);
    int index = 0;
	  while(index<10)
    {
	    pthread_mutex_lock(&my_mutex);
	    
      // wait for free space in fifo
	    if(((struct List*)arg)->elements_number >= MAX_ELEMS)
	            pthread_cond_wait(&buff_cond_empty, &my_mutex);
      
      push_back(t, index);
	    
      // there is no space for write
	    if(((struct List*)arg)->elements_number >= MAX_ELEMS)
                    pthread_cond_signal(&buff_cond_full);
	    
      pthread_mutex_unlock(&my_mutex);
      ++index;
    }
	  pthread_exit(NULL);
}

void* fun_read(void* arg)
{   struct List* t = ((struct List*)arg);
  
    int element = 0;
	  while(1)
    {
	    pthread_mutex_lock(&my_mutex);
	    
      // wait for for something to read in fifo
	    if(((struct List*)arg)->elements_number <= 0)
	            pthread_cond_wait(&buff_cond_full, &my_mutex);
      
      element = pop_front(t);
      printf("Element value: %d .\n", element);
	    
      // there is nothing to write
	    if(((struct List*)arg)->elements_number <= 0)
                    pthread_cond_signal(&buff_cond_empty);
	    
      pthread_mutex_unlock(&my_mutex);
    }
	  pthread_exit(NULL);
}
 
 