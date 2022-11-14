#include "stdint.h"
#include "sys/queue.h"

void stailq_demo()
{
  // Заголовок односвязной хвостовой очереди
  STAILQ_HEAD(stailhead, entry) head = STAILQ_HEAD_INITIALIZER(head);
  struct stailhead *headp;                

  // Объявляем структуру для хранения данных
  struct entry {
    uint32_t data1;
    uint32_t data2;
    uint32_t data3;
    char string[32];
    STAILQ_ENTRY(entry) next;
  } *n1, *n2, *n3, *np, *np_temp;

  // Инициализация очереди
  STAILQ_INIT(&head);                     

  // Добавляем элемент в самое начало очереди
  n1 = malloc(sizeof(struct entry));
  STAILQ_INSERT_HEAD(&head, n1, next);

  // Добавляем элемент в самый конец очереди
  n1 = malloc(sizeof(struct entry));
  STAILQ_INSERT_TAIL(&head, n1, next);

  // Добавляем элемент после указанного
  n2 = malloc(sizeof(struct entry));
  STAILQ_INSERT_AFTER(&head, n1, n2, next);
                                          
  // Удаление элемента из списка
  STAILQ_REMOVE(&head, n2, entry, next);
  free(n2);

  // Удаление элемента из начала списка        
  n3 = STAILQ_FIRST(&head);
  STAILQ_REMOVE_HEAD(&head, next);
  free(n3);
                                        
  // Сканирование вперед
  STAILQ_FOREACH(np, &head, next) {
    // Что-то делаем
    // np-> ...
  };

  // Безопасное сканирование с удалением элементов                  
  STAILQ_FOREACH_SAFE(np, &head, next, np_temp) {
     STAILQ_REMOVE(&head, np, entry, next);
     free(np);
  };
                                          
  // Ещё один способ удаления элементов - с головы
  while (!STAILQ_EMPTY(&head)) {
    n1 = STAILQ_FIRST(&head);
    STAILQ_REMOVE_HEAD(&head, next);
    free(n1);
  }
                                        
  // Быстрое удаление
  n1 = STAILQ_FIRST(&head);
  while (n1 != NULL) {
    n2 = STAILQ_NEXT(n1, next);
    free(n1);
    n1 = n2;
  };
  STAILQ_INIT(&head);
}

void list_demo()
{
  // Заголовок списка
  LIST_HEAD(listhead, entry) head = LIST_HEAD_INITIALIZER(head);
  struct listhead *headp;                

  // Объявляем структуру для хранения данных
  struct entry {
    uint32_t data1;
    uint32_t data2;
    uint32_t data3;
    char string[32];
    LIST_ENTRY(entry) next;
  } *n1, *n2, *n3, *np, *np_temp;

  // Инициализация списка
  LIST_INIT(&head);                     

  // Добавляем элемент в самое начало списка
  n1 = malloc(sizeof(struct entry));
  LIST_INSERT_HEAD(&head, n1, next);

  // Добавляем элемент после указанного
  n2 = malloc(sizeof(struct entry));
  LIST_INSERT_AFTER(n1, n2, next);
                                          
  // Добавляем элемент перед указанным
  n3 = malloc(sizeof(struct entry));
  LIST_INSERT_BEFORE(n2, n3, next);

  // Удаление элемента из списка
  LIST_REMOVE(n2, next);
  free(n2);

  // Сканирование вперед
  LIST_FOREACH(np, &head, next) {
    // Что-то делаем
    // np-> ...
  };

  // Безопасное сканирование с удалением элементов                  
  LIST_FOREACH_SAFE(np, &head, next, np_temp) {
     LIST_REMOVE(np, next);
     free(np);
  };
                                          
  // Ещё один способ удаления элементов - с головы
  while (!LIST_EMPTY(&head)) {
    n1 = LIST_FIRST(&head);
    LIST_REMOVE(n1, next);
    free(n1);
  }
                                        
  // Быстрое удаление
  n1 = LIST_FIRST(&head);
  while (n1 != NULL) {
    n2 = LIST_NEXT(n1, next);
    free(n1);
    n1 = n2;
  };
  LIST_INIT(&head);
}

void tailq_demo()
{
  // Заголовок хвостовой очереди
  TAILQ_HEAD(tailhead, entry) head = TAILQ_HEAD_INITIALIZER(head);
  struct tailhead *headp;                

  // Объявляем структуру для хранения данных
  struct entry {
    uint32_t data1;
    uint32_t data2;
    uint32_t data3;
    char string[32];
    TAILQ_ENTRY(entry) next;
  } *n1, *n2, *n3, *np, *np_temp;

  // Инициализация очереди
  TAILQ_INIT(&head);                     

  // Добавляем элемент в самое начало очереди
  n1 = malloc(sizeof(struct entry));
  TAILQ_INSERT_HEAD(&head, n1, next);

  // Добавляем элемент в самый конец очереди
  n1 = malloc(sizeof(struct entry));
  TAILQ_INSERT_TAIL(&head, n1, next);

  // Добавляем элемент после указанного
  n2 = malloc(sizeof(struct entry));
  TAILQ_INSERT_AFTER(&head, n1, n2, next);
                                          
  // Добавляем элемент перед указанным
  n3 = malloc(sizeof(struct entry));
  TAILQ_INSERT_BEFORE(n2, n3, next);

  // Удаление элемента из списка
  TAILQ_REMOVE(&head, n3, next);
  free(n3);

  // Сканирование вперед
  TAILQ_FOREACH(np, &head, next) {
    // Что-то делаем
    // np-> ...
  };

  // Безопасное сканирование с удалением элементов                  
  TAILQ_FOREACH_SAFE(np, &head, next, np_temp) {
     TAILQ_REMOVE(&head, np, next);
     free(np);
  };
                                          
  // Ещё один способ удаления элементов - с головы
  while (!TAILQ_EMPTY(&head)) {
    n1 = TAILQ_FIRST(&head);
    TAILQ_REMOVE(&head, n1, next);
    free(n1);
  }
                                        
  // Быстрое удаление
  n1 = TAILQ_FIRST(&head);
  while (n1 != NULL) {
    n2 = TAILQ_NEXT(n1, next);
    free(n1);
    n1 = n2;
  };
  TAILQ_INIT(&head);
}

void app_main() 
{
}
