#define LOG_FLAG 0

#define PANIC_1 (LOG_FLAG==1)
#define PANIC_2 (LOG_FLAG==2)
#define PANIC_3 (LOG_FLAG==3)
#define PANIC_4 (LOG_FLAG==4)
#define PANIC_5 (LOG_FLAG==5)

#define WRITE (PANIC_1 && PANIC_2 && PANIC_3 && PANIC_4 && PANIC_5)