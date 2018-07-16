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
  int full, empty;
} buffer_input, buffer_output;

pthread_mutex_t input_mutex, output_mutex, tool_mutex, wait_mutex;
pthread_cond_t condc1, condc2, condp1, condp2, condt1;

int produced_material[3];
int not_used_material[4];
int check_produce_material[3];
int out_buffer[3];
int num_material[3];
int pause_signal = 0;
int num_operator = 3;
int num_wait = 0;
int num_deadlock = 0;
int num_tool = 3;

buffer_input* input;
buffer_output* output;



int check_buffer(int item)
{
  int num = 0;
  if (item == produced_material[0]) // these two materials are the same
    return 0;
  if (item == produced_material[1] && produced_material[0] == produced_material[2])
  // produce the same product becuase of same material
    return 0;
  for (int j=1; j==3; j++)
  {
    if (not_used_material[item] - not_used_material[j] > 5)
      num++;
    if ( num == 2 ) //a material's maximum number
    {
      if(produced_material[1]!=item && produced_material[2]!=item)
        return 1;
      return 0;
    }
  }
  return 1;
}

// get the number of material
int get_num_material(buffer_input* q)
{
  int num1 = q -> first;
  int num2 = q -> last;
  if (num1 == num2)
    return 0;
  if (num1 - num2 == 10 || num2 - num1 == 1)
    return -1;
  if (num1 > num2)
    return (num1 - num2);
  else
    return (num1 + 11 - num2);

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

int get_material(buffer_input* q)
{
  int i = q -> buffer[(q -> last)++];
  if (q -> last == 11)
    q -> last = 0;
  return i;
}

int check_produce(int x, int y)
{

  int num1,num2,num;
  if (x > y)
    num1 = x;
  else
    num1 = y;
  num2 = x+y-num1;
  if (num1 == 2 && num2 == 1)
    num = check_produce_material[0];
  if (num1 == 3 && num2 == 1)
    num = check_produce_material[1];
  if (num1 == 3 && num2 == 2)
    num = check_produce_material[2];
  return num;
}

void produce(int x, int y)
{
  int num1, num2, s1, s2, s3; // s1 is max; s2 is second max; s3 is the third one
  if (x > y)
    num1 = x;
  else
    num1 = y;
  num2 = x+y-num1;
  check_produce_material[0] = 1;
  check_produce_material[1] = 1;
  check_produce_material[2] = 1;
  if (num1 == 2 && num2 == 1)
  {
    remove_material(output,1);
    out_buffer[0]++;
    check_produce_material[0] = 0;
  }
  else if (num1 == 3 && num2 == 1)
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
   if (out_buffer[0] >= out_buffer[1] && out_buffer[0] >= out_buffer[2])
   {
     s1 = 0;
     if (out_buffer[1] > out_buffer[2])
     {
       s2 = 1;
       s3 = 2;
     }
     else
     {
       s2 = 2;
       s3 = 1;
     }
   }
   else if (out_buffer[1] >= out_buffer[2])
   {
     s1 = 1;
     if (out_buffer[0] >= out_buffer[2])
     {
       s2 = 0;
       s3 = 2;
     }
     else
     {
       s2 = 2;
       s3 = 0;
     }
   }
   else
   {
      s1 = 2;
      if (out_buffer[0] >= out_buffer[1])
      {
        s2 = 0;
        s3 = 1;
      }
      else
      {
        s2 = 1;
        s3 = 0;
      }
   }
  if (out_buffer[s1] - out_buffer[s3] >= 9)
    check_produce_material[s1] = 0;
  if (out_buffer[s2] - out_buffer[s3] >= 9)
    check_produce_material[s2] = 0;
}

void* generator (int num)
{

  while (TRUE)
  {
      pthread_mutex_lock(&input_mutex);
      while (get_num_material(input) == -1 || check_buffer(num) == 0)
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


void* operation(void* ptr)
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
    num_tool --;
    // get one tool
    pthread_mutex_unlock(&tool_mutex);
    pthread_mutex_lock(&input_mutex);
    while (get_num_material(input) == 0)
    {
      pthread_cond_wait(&condc2, &input_mutex);
    }
    num1 = get_material(input);
    // get one material
    pthread_cond_signal(&condp2);
    pthread_mutex_unlock(&input_mutex);
    pthread_mutex_lock(&input_mutex);
    while (get_num_material(input) == 0)
    {
      pthread_cond_wait(&condc2, &input_mutex);
    }
    num2 = get_material(input);
    // get anothher material
    pthread_cond_wait(&condp2, &input_mutex);
    pthread_mutex_unlock(&input_mutex);
    // there are two same materials or products, change one of them
    while (num1 == num2)
    {
      pthread_mutex_lock(&input_mutex);
      while (get_num_material(input) == 0)
      {
        pthread_cond_wait(&condc2, &input_mutex);
      }
      num2 = get_material(input);
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
    pthread_mutex_lock(&output_mutex);

    //wait
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
    if (deadlock_flag == 1)
    {
      num_wait--;
    }
    pthread_mutex_unlock(&wait_mutex);
    while(pause_signal)
    {
      usleep(100);
    }
  }
  pthread_exit(0);
}

void* dynamic_output(void* item)
{
  while (TRUE)
  {
    while (pause_signal)
    {
      usleep(100);
    }
    pthread_mutex_lock(&input_mutex);
    printf("material1: %d, material2: %d, material3: %d\n", num_material[0], num_material[1], num_material[2]);
    int i = get_num_material(input);
    if (i == -1)
    {
      i = 10;
    }
    printf("buffer size: %d\n", i);
    printf("input buffer: ");
    i = input -> first;
    while (i != input -> last)
    {
      i--;
      if (i == -1)
      {
        i = 10;
      }
      printf("%d", input -> buffer[i] );
    }
    pthread_mutex_unlock(&input_mutex);
    printf("\n\n");
    printf("product1: %d, product2: %d, product3: %d\n",out_buffer[0], out_buffer[1], out_buffer[2] );
    pthread_mutex_lock(&output_mutex);
    printf("First ten of output buffer: ");
    i = output -> first;
    while (i != output -> last)
    {
      i--;
      if (i == -1)
      {
        i =10;
      }
      printf("%d",output -> buffer[i] );
    }
    pthread_mutex_unlock(&output_mutex);
    printf("Tools available now: %d\n", num_tool);
    printf("%d process cannot output the produce now and are waiting\n", num_wait);
    printf("Deadlock happend %d times\n", num_deadlock );
    printf("\n\n\nPress  key \'CTRL + Z\' to pause the program (I deal with this signal with my own function)\nPress  key \'CTRL + C\' to terminate this program.(I deal with this signal with my own function)\n");
    for (int j=0; j==3; j++ )
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
			printf("Program paused.\nTo resume, Press \'CTRL + Z\'\nTo Terminate, press \'CTRL + C\'\n");
    }
  }
}

void do_quit(int sigal)
{
  if (sigal == SIGTSTP)
  {
    printf("Terminated\n");
    exit(0);
  }
}

int main(void)
{
  pthread_t g1, g2, g3;
  pthread_t operator[11];
  pthread_t dynamic;
  int i;

  pthread_mutex_init(&input_mutex, NULL);
  pthread_mutex_init(&output_mutex, NULL);
  pthread_mutex_init(&wait_mutex, NULL);
  pthread_mutex_init(&tool_mutex, NULL);
  pthread_cond_init(&condc2, NULL);
  pthread_cond_init(&condc1, NULL);
  pthread_cond_init(&condp2, NULL);
  pthread_cond_init(&condp1, NULL);
  pthread_cond_init(&condt1, NULL);

  not_used_material[0] = 0;
  not_used_material[1] = 0;
  not_used_material[2] = 0;

  produced_material[0] = 0;
  produced_material[1] = 0;
  produced_material[2] = 0;

  output = (buffer_output*) malloc(sizeof(buffer_output));
  output -> last = 0;
  output -> first = 0;
  input = (buffer_input*) malloc (sizeof(buffer_input));
  input -> last = 0;
  input -> first = 0;

  num_material[0] = 0;
  num_material[1] = 0;
  num_material[2] = 0;

  out_buffer[0] = 0;
  out_buffer[1] = 0;
  out_buffer[2] = 0;

  check_produce_material[0] = 1;
  check_produce_material[1] = 1;
  check_produce_material[2] = 1;

  //wait
  char ch;
  while (ch != '1' && ch != '2')
  {
    ch = getchar();
  }
  if (ch == '2')
  {
    scanf("%d\n", &i);
    if (i >= 2 && i < 100)
      num_tool = i;
    else
      num_tool = 3;
  }
  if (i >= 1 && i <= 10)
    num_operator = i;
  else
    num_operator = 3;

  //run
  signal(SIGINT, do_quit);
  signal(SIGTSTP, do_pause);
  pthread_create(&dynamic, NULL, dynamic_output, NULL);
  pthread_create(&g1, NULL, generator, 1);
  pthread_create(&g2, NULL, generator, 2);
  pthread_create(&g3, NULL, generator, 3);
  for (i = 1; i <= num_operator; i++)
  {
    pthread_create(&operator[i], NULL, operation, NULL);
    pthread_join(operator[i], NULL);
  }
  pthread_join(g1, NULL);
  pthread_join(g2, NULL);
  pthread_join(g3, NULL);
  pthread_join(dynamic, NULL);
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
