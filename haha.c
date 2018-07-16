#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <signal.h>
#include <unistd.h>

# define QUEUE_OUTPUT 10000000

# define BUFFER_SIZE 11
# define TRUE 1

typedef struct
{
  int buffer[BUFFER_SIZE];
  long first, last;
} buffer_input, buffer_output;

pthread_mutex_t input_mutex, output_mutex, tool_mutex, wait_mutex;
pthread_cond_t condc1, condc2, condp1, condp2, condt1;

int produced_material[3] = {0,0,0};
int not_used_material[4];
int check_produce_material[3] = {1,1,1};
int out_buffer[3] = {0,0,0};
int num_material[4];
int pause_signal = 0;
int num_operator = 3;
int num_wait = 0;
int num_deadlock = 0;
int num_tool = 3;

buffer_input* input;
buffer_output* output;


void buffer_init(void)
{
	pthread_mutex_t input_mutex = PTHREAD_MUTEX_INITIALIZER;
  pthread_mutex_t output_mutex = PTHREAD_MUTEX_INITIALIZER;
  pthread_mutex_t wait_mutex = PTHREAD_MUTEX_INITIALIZER;
  pthread_mutex_t tool_mutex = PTHREAD_MUTEX_INITIALIZER;
	pthread_cond_t condc1 = PTHREAD_COND_INITIALIZER;
	pthread_cond_t condc2 = PTHREAD_COND_INITIALIZER;
  pthread_cond_t condp1 = PTHREAD_COND_INITIALIZER;
  pthread_cond_t condp2 = PTHREAD_COND_INITIALIZER;
  pthread_cond_t condt1 = PTHREAD_COND_INITIALIZER;

  not_used_material[1] = 0;
  not_used_material[2] = 0;
  not_used_material[3] = 0;

  num_material[1] = 0;
  num_material[2] = 0;
  num_material[3] = 0;

  output = (buffer_output*) malloc(sizeof(buffer_output));
  output -> first = 0;
  output -> last = 0;
  input = (buffer_input*) malloc (sizeof(buffer_input));
  input -> first = 0;
  input -> last = 0;

	printf("setup finished\n");
}

static inline unsigned long long max(unsigned long long a, unsigned long long b)
{
  return (a > b) ? a : b;
}

static inline unsigned long long min(unsigned long long a, unsigned long long b)
{
  return (a > b) ? b : a;
}



// get the number of material
int get_num_material(buffer_input* q)
{

  if (q -> first == q -> last)
    return 0;
  if (q -> first - q -> last == 10 || q -> last - q -> first == 1)
    return -1;
  if (q -> first > q -> last)
    return (q -> first - q -> last);
  else
    return (q -> first + 11 - q -> last);
}


void insert_material(buffer_input* q, int num)
{
  q -> buffer[(q -> first)++] = num;
  produced_material[2] = produced_material[1];
  produced_material[1] = produced_material[0];
  produced_material[0] = num;
  if ( q -> first == 11)
    q -> first = 0;
}

void remove_material(buffer_output* q, int num)
{

  if (q -> first - q -> last == 10)
    q -> last = 1;
  if (q -> last - q -> first == 1)
  {
    q -> last ++;
    if (q -> last == 11)
      q -> last = 0;
  }
  q -> buffer[(q -> first) ++] = num;
  produced_material[2] = produced_material[1];
  produced_material[1] = produced_material[0];
  produced_material[0] = num;
  if ( q -> first == 11)
    q -> first = 0;
}



void produce(int x, int y)
{

  int n1 = max(x,y);
  int n2 = min(x,y);



  if (n1 == 2 && n2 == 1)
  {
    remove_material(output,1);
    out_buffer[0]++;
    check_produce_material[0] = 0;
  }
  else if (n1 == 3 && n2 == 1)
  {
    remove_material(output,2);
    out_buffer[1]++;
    check_produce_material[1] = 0;
  }
  else
  {
    remove_material(output,3);
    out_buffer[2]++;
    check_produce_material[2] = 0;
  }

  // check if the difference between two different materials less tham 10

  if (out_buffer[0] - out_buffer[1] > 10 || out_buffer[0] - out_buffer[2] > 10)
    check_produce_material[0] = 0;
  if (out_buffer[1] - out_buffer[2] > 10 || out_buffer[1] - out_buffer[0] > 10)
    check_produce_material[1] = 0;
  if (out_buffer[2] - out_buffer[0] > 10 || out_buffer[2]-out_buffer[1] > 10)
    check_produce_material[2] = 0;
}


int check_buffer(int item)
{
  if (item == produced_material[0]) // these two materials are the same
    return 0;
  if (item == produced_material[1] && produced_material[0] == produced_material[2])
  // produce the same product becuase of same material
    return 0;

  if (not_used_material[item] - not_used_material[1] > 5 ||
      not_used_material[item] - not_used_material[2] > 5 ||
      not_used_material[item] - not_used_material[3] > 5)
      {
        int num = 1;
        for (int j = 1; j<4; j++)
        {
          if (not_used_material[item] > not_used_material[j])
            num++;
        }
        if ( num == 3 ) //a material's maximum number
        {
          if(produced_material[1]!=item && produced_material[2]!=item)
            return 1;
          return 0;
        }
      }
  return 1;
}



void* generator1(void* item)
{
  int num = 1;
  while (TRUE)
  {
      pthread_mutex_lock(&input_mutex);
      while (get_num_material(input) == -1 || !check_buffer(num))
      {
        if (get_num_material(input) == -1)
          pthread_cond_wait(&condp2, &input_mutex);
        else
        {
          pthread_cond_signal(&condp2);
          pthread_cond_signal(&condp1);
          pthread_cond_wait(&condp1, &input_mutex);
        }
      }
      insert_material(input,num);
      num_material[num]++;
      not_used_material[num]++;
      pthread_cond_signal(&condc2);
      pthread_cond_signal(&condp1);
      pthread_mutex_unlock(&input_mutex);
      while(pause_signal)
      {
        usleep(100);
      }
  }
  pthread_exit(0);
}



void* generator2(void* item)
{
  int num = 2;
  while (TRUE)
  {
      pthread_mutex_lock(&input_mutex);
      while (get_num_material(input) == -1 || !check_buffer(num))
      {
        if (get_num_material(input) == -1)
          pthread_cond_wait(&condp2, &input_mutex);
        else
        {
          pthread_cond_signal(&condp2);
          pthread_cond_signal(&condp1);
          pthread_cond_wait(&condp1, &input_mutex);
        }
      }
      insert_material(input,num);
      num_material[num]++;
      not_used_material[num]++;
      pthread_cond_signal(&condc2);
      pthread_cond_signal(&condp1);
      pthread_mutex_unlock(&input_mutex);
      while(pause_signal)
      {
        usleep(100);
      }
  }
  pthread_exit(0);
}



void* generator3(void* item)
{
  int num = 3;
  while (TRUE)
  {
      pthread_mutex_lock(&input_mutex);
      while (get_num_material(input) == -1 || !check_buffer(num))
      {
        if (get_num_material(input) == -1)
          pthread_cond_wait(&condp2, &input_mutex);
        else
        {
          pthread_cond_signal(&condp2);
          pthread_cond_signal(&condp1);
          pthread_cond_wait(&condp1, &input_mutex);
        }
      }
      insert_material(input,num);
      num_material[num]++;
      not_used_material[num]++;
      pthread_cond_signal(&condc2);
      pthread_cond_signal(&condp1);
      pthread_mutex_unlock(&input_mutex);
      while(pause_signal)
        usleep(100);
  }
  pthread_exit(0);
}



int get_material(buffer_input* q)
{
  int i = q -> buffer[(q -> last)++];
  if (q -> last == 11)
    q -> last = 0;
  return i;
}

int material()
{
  pthread_mutex_lock(&input_mutex);
  while (get_num_material(input) == 0)
  {
    pthread_cond_wait(&condc2, &input_mutex);
  }
  int num = get_material(input);
  return num;
}



int check_produce(int x, int y)
{
  check_produce_material[0]=1;
	check_produce_material[1]=1;
	check_produce_material[2]=1;

  int num1,num2,num;
  num1 = max(x,y);
  num2 = min(x,y);
  if (num1 == 2 && num2 == 1)
    num = check_produce_material[0];
  if (num1 == 3 && num2 == 1)
    num = check_produce_material[1];
  if (num1 == 3 && num2 == 2)
    num = check_produce_material[2];
  return num;
}


void* operation(void* item)
{
  int num1,num2;
  while (TRUE)
  {
    int deadlock_flag = 0;
    pthread_mutex_lock(&tool_mutex);
    while (num_tool < 2)
      pthread_cond_wait(&condt1, &tool_mutex);
    num_tool--;
    // get one tool
    pthread_mutex_unlock(&tool_mutex);
    pthread_mutex_lock(&tool_mutex);
    if (num_tool == 0)
    {
      num_tool ++;
      pthread_mutex_unlock(&tool_mutex);
      continue;
    }
    num_tool --; // one tool
    pthread_mutex_unlock(&tool_mutex);
    num1 = material();  // one material

    pthread_cond_signal(&condp2);
    pthread_mutex_unlock(&input_mutex);
    num2 = material(); //another material

    pthread_cond_signal(&condp2);
    pthread_mutex_unlock(&input_mutex);

    // there are two same materials or products, change one of them
    while (num1 == num2)
    {
      pthread_mutex_lock(&input_mutex);
			while (get_num_material(input)==0)
        pthread_cond_wait(&condc2,&input_mutex);
			num2=get_material(input);
			insert_material(input,num1);
			pthread_mutex_unlock(&input_mutex);
    }

    //within a limited time varied from 0.01 second to 1 second
    usleep(1000*(10+rand()%991));

    //put tool back
    pthread_mutex_lock(&tool_mutex);
    num_tool +=2;
    pthread_cond_signal(&condt1);
    pthread_mutex_unlock(&tool_mutex);

    //wait
    pthread_mutex_lock(&output_mutex);
    if (!check_produce(num1,num2))
    {
      pthread_mutex_lock(&wait_mutex);
      num_wait++;
      if (num_wait == num_operator)
      {
        num_deadlock++;
        num_wait --;

        pthread_mutex_lock(&input_mutex);
        not_used_material[num1] --;
        not_used_material[num2] --;
        pthread_mutex_unlock(&input_mutex);

        pthread_mutex_unlock(&wait_mutex);
        pthread_mutex_unlock(&output_mutex);
        continue;
      }
      pthread_mutex_unlock(&wait_mutex);
      deadlock_flag = 1;
    }

    while (!check_produce(num1,num2))
    {
      pthread_cond_wait(&condc1, &output_mutex);
    }
    pthread_mutex_lock(&wait_mutex);
    produce(num1,num2);
    pthread_cond_signal(&condc1);
    pthread_mutex_unlock(&output_mutex);
    if (deadlock_flag)
    {
      num_wait--;
    }
    pthread_mutex_unlock(&wait_mutex);
    while(pause_signal)
      usleep(100);
  }
  pthread_exit(0);
}


void* dynamic_output(void* item)
{
  while (TRUE)
  {
    while (pause_signal)
      usleep(100);

    printf("--MATERIALS--\n" );
    pthread_mutex_lock(&input_mutex);
    printf("material1: %d, material2: %d, material3: %d\n", num_material[1], num_material[2], num_material[3]);
    int i = get_num_material(input);

    if (i == -1)
      i = 10;

    printf("Buffer Size: %d", i);

    printf("\nInput Buffer: ");
    i = input -> first;
    while (i != input -> last)
    {
      i--;
      if (i == -1)
        i = 10;
      printf("%d", input -> buffer[i] );
    }

    pthread_mutex_unlock(&input_mutex);
    printf("\n\n");
    printf("--PRODUCT--\n" );
    printf("product1: %d, product2: %d, product3: %d\n",out_buffer[0], out_buffer[1], out_buffer[2] );

    pthread_mutex_lock(&output_mutex);
    printf("Output Buffer: ");


    i = output -> first;
    while (i != output -> last)
    {
      i--;
      if (i == -1)
        i =10;
      printf("%d",output -> buffer[i] );
    }
    pthread_mutex_unlock(&output_mutex);

    printf("\n\n");
    printf("Tools available now: %d\n", num_tool);
    printf("%d process are waiting \n", num_wait);
    printf("Deadlock happend %d times\n", num_deadlock );
    printf("\n\n\nPress  key \'CTRL + Z\' to pause the program and press again for resuming\n");
    printf("\nPress  key \'CTRL + C\' to quit this program.\n");
    printf("\n\n");

    for (int j=0; j<4; j++ )
    {
      int total_buffer = 0;
      total_buffer = total_buffer + out_buffer[j];
      if (total_buffer > QUEUE_OUTPUT)
      {
        printf("output buffer is larger than the maximum output %d\n", QUEUE_OUTPUT);
        exit(0);
      }
    }
    sleep(1);
  }
  pthread_exit(0);
}

void do_pause(int sigal)
{
  if (sigal == SIGTSTP)
  {
    pause_signal = 1 - pause_signal;
    if (pause_signal == 1)
    {
      printf("\n\n\n");
			printf("Program paused\n\n");
      printf("To resume, Press \'CTRL + Z\'\n.To quiz, press \'CTRL + C\'\n");
    }
  }
}

void do_quit(int sigal)
{
  if (sigal == SIGINT)
  {
    printf("\n");
    printf("Program exited\n\n");
    exit(0);
  }
}

int main(void)
{
  pthread_t g1, g2, g3;
  pthread_t operator[11];
  pthread_t dynamic;
  int i;

  buffer_init();
  printf("\n\nCase 1 is default(3 tools and operators) and Case 2 is adjustable ");
  printf("\nChoose Case 1 or Case 2: ");

  char c=getchar();
	while(c!='1' && c!='2')
	{
		printf("\n\nChoose Case 1 or Case 2: (Input only 1 or 2)");
		c=getchar();
	}
	if(c=='2')
	{
		printf("Input the tool number between 2 and 20 (other number will default to 3):");
		scanf("%d",&i);
		if (i>=2&&i<=20)
      num_tool = i;
    else
     num_tool = 3;

		printf("\n Input the operator number between 1 to 10 (other number will default to 3):");
		scanf("%d",&i);
		if (i>=1&&i<=10)
      num_operator = i;
    else
      num_operator = 3;
	}


  //run
  signal(SIGINT, do_quit);
  signal(SIGTSTP, do_pause);
  pthread_create(&dynamic, NULL, dynamic_output, NULL);
  pthread_create(&g1, NULL, generator1, NULL);
  pthread_create(&g2, NULL, generator2, NULL);
  pthread_create(&g3, NULL, generator3, NULL);

  for (i = 1; i <= num_operator; i++)
    pthread_create(&operator[i], NULL, operation, NULL);

  pthread_join(g1, NULL);
  pthread_join(g2, NULL);
  pthread_join(g3, NULL);
  pthread_join(dynamic, NULL);

  for(i=1; i<=num_operator; i++)
    pthread_join(operator[i],NULL);



  pthread_mutex_destroy(&input_mutex);
  pthread_mutex_destroy(&output_mutex);
  pthread_mutex_destroy(&tool_mutex);
  pthread_mutex_destroy(&wait_mutex);

  pthread_cond_destroy(&condp2);
  pthread_cond_destroy(&condp1);
  pthread_cond_destroy(&condc2);
  pthread_cond_destroy(&condc1);
  pthread_cond_destroy(&condt1);
}
