#include "sys/queue.h"

/**************************************************************
 * Пример работы с односвязными списками SLIST
 **************************************************************/
void slist_example() 
{
  // Объявляем структуру для хранения каких-либо данных
  struct entry {
    // вы можете добавить сюда любые данные
    int data1;
    int data2;
    // главное - не забудьте про SLIST_ENTRY(entry)
    SLIST_ENTRY(entry) next;
  } *n1, *n2, *n3, *np, *np_temp;

  // Объявляем структуру-заголовок списка 
  SLIST_HEAD(slisthead, entry) head = SLIST_HEAD_INITIALIZER(head);

  // Заголовок списка
  struct slisthead *headp;         

  // Инициализация списка
  SLIST_INIT(&head);

  // Добавляем первый элемент (голову)
  n1 = malloc(sizeof(struct entry));
  SLIST_INSERT_HEAD(&head, n1, next);

  // Добавляем следующий элемент 
  n2 = malloc(sizeof(struct entry));
  SLIST_INSERT_AFTER(n1, n2, next);

  // Удаляем любой указанный элемент
  SLIST_REMOVE(&head, n2, entry, next);
  free(n2);

  // Удаление первого элемента
  n3 = SLIST_FIRST(&head);
  SLIST_REMOVE_HEAD(&head, next);
  free(n3);
  
  // Сканирование вперед
  SLIST_FOREACH(np, &head, next) {
    // Что-то делаем ...
    // np->
  }
          
  // Безопасное сканирование вперед и удаление элементов
  SLIST_FOREACH_SAFE(np, &head, next, np_temp) {
    SLIST_REMOVE(&head, np, entry, next);
    free(np);
  }

  // Ещё один способ удаления всех элементов - с головы
  while (!SLIST_EMPTY(&head)) {
    n1 = SLIST_FIRST(&head);
    SLIST_REMOVE_HEAD(&head, next);
    free(n1);
  }
}

void app_main() 
{
  // Это не рабочие примеры
}
