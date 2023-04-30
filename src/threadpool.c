#define _GNU_SOURCE
#include <sched.h>
#include "threadpool.h"

#include <signal.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/prctl.h>
#include <unistd.h>
#include <math.h>

#include "filter.h"
#include "list.h"

#include <pthread.h>

#define TIMEOUT_SEC 5


void* worker(void* arg) {
  /*
     * À compléter:
     *
     * Attendre que tous les fils d'exécution soient démarrés en utilisant une
     * barrière. Ceci permet de s'assurer que tous les fils d'exécution sont en
     * fonction quand threadpool_create(). ✅
     *
     * Dans une boucle infinie, on vérifie s'il y a des tâches dans la liste.
     * Si c'est le cas, on prend un élément de la liste et on exécute la fonction.
     * La fonction est l'argument sont spécifiés dans l'item de la liste: ✅
     *
     *    task->func(task->arg); ✅
     *
     * Si la liste est vide, on doit attendre qu'on y ajoute un élément en
     * utilisant pthread_cond_wait(). ✅
     *
     * Attention: la liste peut être corrompue si on l'accède dans plusieurs
     * fils d'exécution en même temps. Il faut donc protéger l'accès avec un
     * verrou. ✅
     *
     * La boucle doit se terminer si pool->running est faux ET que la liste
     * pool->task_list est vide. ✅
     *
     * Bonus: insérer dans cette fonction l'instrumentation pour savoir le nombre
     * de fils d'exécution qui travaillent (i.e. exécutent task->func()) en
     * fonction du temps.
   */

  // Récupère le pool et initialise les arguments du worker.
  struct worker_arg* w = arg;
  struct pool* pool = w->pool;

  // Attendez que tous les threads de travail soient créés avant de continuer.
  pthread_barrier_wait(&pool->ready);

  // Démarre une boucle infinie.
  while (1) {

    // Acquérir le verrou pour accéder à la liste des tâches.
    pthread_mutex_lock(&pool->lock);

    // Vérifie si la liste des tâches est vide et si le pool n'est plus en cours
    // d'exécution.
    if (list_empty(pool->task_list) && !pool->running) {
      // Si oui, relâchez le verrou et cassez la boucle.
      pthread_mutex_unlock(&pool->lock);
      break;
    }

    // Si la liste des tâches est vide mais que le pool est toujours en cours
    // d'exécution, attendez un signal indiquant qu'une tâche a été ajoutée à la
    // liste.
    while (list_empty(pool->task_list) && pool->running) {
      int ret = pthread_cond_wait(&pool->work_todo, &pool->lock);
      if (ret != 0) {
        perror("pthread_cond_wait");
        pthread_mutex_unlock(&pool->lock);
        return NULL;
      }

      // Si la liste des tâches est vide et que le pool n'est plus en cours
      // d'exécution. relâchez le verrou et revenez.
      if (list_empty(pool->task_list) && !pool->running) {
        pthread_mutex_unlock(&pool->lock);
        return NULL;
      }
    }

    // Récupère la première tâche de la liste.
    struct task* task = (struct task*)list_pop_front(pool->task_list)->data;

    // Relâchez le verrou pour permettre à d'autres threads d'accéder à la liste
    // des tâches.
    pthread_mutex_unlock(&pool->lock);

    // Exécute la fonction de tâche avec son argument.
    task->func(task->arg);

    pthread_mutex_lock(&pool->lock);

    // Signale qu'une tâche est terminée et peut être supprimée de la liste des
    // tâches.
    free(task);

    // Signale qu'une tâche est terminée et peut être supprimée de la liste des
    // tâches.
    int ret = pthread_cond_broadcast(&pool->work_done);
    if (ret != 0) {
      perror("pthread_cond_broadcast");
      pthread_mutex_unlock(&pool->lock);
      return NULL;
    }

    // Relâchez le verrou
    pthread_mutex_unlock(&pool->lock);


  }

  return NULL;
}


struct pool* threadpool_create(int num) {
  /*
     * À compléter:
     *
     * Allouer la structure que la fonction retourne struct pool*
     *
     * Passer en revue tous les champs et initialiser correctement, en allouant la
     * mémoire au besoin.
     *
     * Initialiser le verrou lock ✅
     * Initialiser les conditions work_todo et work_done ✅
     * Initialiser la barrière ✅
     * Créer la liste de travail task_list ✅
     *
     * Démarrer les fils d'exécution ✅
     *
     * Attendre la confirmation qu'ils sont tous démarrés avec la barrière. La
     * fonction doit retourner uniquement quand tous les fils ont été démarrés. ✅
     *
     * Retourner le pointeur vers la structure. ✅
   */
  // Threads affinity variables
  int num_cores = sysconf(_SC_NPROCESSORS_ONLN);
  int rand_core = 0;
  cpu_set_t cpuset;


  // Allocation mémoire
  struct pool* threadpool = (struct pool*)malloc(sizeof(struct pool));
  if (threadpool == NULL) {
    perror("malloc");
    return NULL;
  }

  threadpool->task_list = list_new(NULL, NULL);
  threadpool->nb_threads = num;
  threadpool->running = 1;

  // Allouer de la mémoire pour les threads et les tableaux args
  threadpool->threads = (pthread_t*)malloc(threadpool->nb_threads * sizeof(pthread_t));
  if (threadpool->threads == NULL) {
    perror("malloc");
    free(threadpool);
    return NULL;
  }

  threadpool->args = (struct worker_arg*)malloc(threadpool->nb_threads * (sizeof (int) + sizeof(struct pool)));
  if (threadpool->args == NULL) {
    perror("malloc");
    free(threadpool->threads);
    free(threadpool);
    return NULL;
  }

  //Initialisation
  if (pthread_mutex_init(&threadpool->lock, NULL) != 0) {
    perror("pthread_mutex_init");
    free(threadpool->args);
    free(threadpool->threads);
    free(threadpool);
    return NULL;
  }
  if (pthread_barrier_init(&threadpool->ready, NULL, threadpool->nb_threads) != 0) {
    perror("pthread_barrier_init");
    pthread_mutex_destroy(&threadpool->lock);
    free(threadpool->args);
    free(threadpool->threads);
    free(threadpool);
    return NULL;
  }
  if (pthread_cond_init(&threadpool->work_todo, NULL) != 0) {
    perror("pthread_cond_init");
    pthread_barrier_destroy(&threadpool->ready);
    pthread_mutex_destroy(&threadpool->lock);
    free(threadpool->args);
    free(threadpool->threads);
    free(threadpool);
    return NULL;
  }
  if (pthread_cond_init(&threadpool->work_done, NULL) != 0) {
    perror("pthread_cond_init");
    pthread_cond_destroy(&threadpool->work_todo);
    pthread_barrier_destroy(&threadpool->ready);
    pthread_mutex_destroy(&threadpool->lock);
    free(threadpool->args);
    free(threadpool->threads);
    free(threadpool);
    return NULL;
  }


  // Initialise les arguments du worker
  for (int i = 0; i < threadpool->nb_threads; ++i) {
    threadpool->args[i].id = i;
    threadpool->args[i].pool = threadpool; //because each thread will be used multiple times and need access to all func
  }

  // Créer chaque thread de travail && le démarrer
  for (int i = 0; i < threadpool->nb_threads; ++i) {
    rand_core = round(rand()%num_cores);

    int thread = pthread_create(&threadpool->threads[i], NULL, worker, (void*)&threadpool->args[i]);
    if(thread) {
      perror("thread");
    }

    CPU_ZERO(&cpuset);
    CPU_SET(rand_core, &cpuset);

    int ret = pthread_setaffinity_np(threadpool->threads[i], sizeof(cpu_set_t), &cpuset);
    if (ret != 0) {
      perror("pthread_setaffinity_np");
    }

  }

  return threadpool;
}

void threadpool_add_task(struct pool* pool, func_t fn, void* arg) {
  /*
     * À compléter:
     *
     * Allouer un struct task, assigner func et arg passés en argument.
     *
     * Ensuite, créer un nouveau noeud de liste pour contenir la tâche créée.
     * Ajouter ce noeuds à la fin de la liste.
     *
     * Signaler la variable de condition pour réveiller au besoin un fil
     * d'exécution.
     *
     * Attention: il faut toujours accéder à la liste avec un verrou pour éviter
     * une condition critique!
   */

  // Ne pas ajouter plus de travail si on doit se terminer
  pthread_mutex_lock(&pool->lock);
  if (!pool->running) {
    pthread_mutex_unlock(&pool->lock);
    return;
  }

  // Alloue de la mémoire pour la tâche et définit sa fonction et son argument
  struct task* new_task = (struct task*)malloc(sizeof(struct task));
  if (!new_task) {
    perror("threadpool_add_task: malloc");
    pthread_mutex_unlock(&pool->lock);
    return;
  }

  new_task->func = fn;
  new_task->arg = arg;

  // Créer un nouveau nœud de liste pour contenir la tâche créée
  struct list_node* new_node =  list_node_new(new_task);

  // Ajouter le nouveau nœud à la fin de la liste
  list_push_back(pool->task_list, new_node);


  // Signale qu'une tâche doit être fait et qu'elle est disponible dans la liste
  pthread_cond_broadcast(&pool->work_todo);
  pthread_mutex_unlock(&pool->lock);
}

void threadpool_join(struct pool* pool) {
  /*
     * À compléter:
     *
     * Mettre pool->running à 0 pour indiquer que l'on désire terminer
     * l'exécution. Signaler la condition de telle sorte que TOUS les fils
     * d'exécution soient avertis.
     *
     * Attendre que tous les fils d'exécution se terminent, puis libérer la
     * mémoire allouée dans threadpool_create().
     *
     * Quand la fonction retourne, la structure pool n'est plus utilisable.
   */

  // Mettre pool->running à 0 pour indiquer que nous voulons terminer

  pthread_mutex_lock(&pool->lock);
  pool->running = 0;
  int nbr_threads = pool->nb_threads;
  pthread_mutex_unlock(&pool->lock);


  // Signalez la variable de condition work_todo pour notifier tous les threads
  if(pthread_cond_broadcast(&pool->work_todo) != 0){
    perror("pthread_cond_broadcast failed");
    exit(EXIT_FAILURE);
  }

  // Attendre que tous les threads se terminent
  for (int i = 0; i < nbr_threads; i++) {
    if(pthread_join(pool->threads[i], NULL) != 0){
      perror("pthread_join failed");
      exit(EXIT_FAILURE);
    }
  }

  // Détruit les objets de synchronisation
  if(pthread_mutex_destroy(&pool->lock) != 0){
    perror("pthread_mutex_destroy failed");
    exit(EXIT_FAILURE);
  }
  if(pthread_cond_destroy(&pool->work_todo) != 0){
    perror("pthread_cond_destroy failed");
    exit(EXIT_FAILURE);
  }
  if(pthread_cond_destroy(&pool->work_done) != 0){
    perror("pthread_cond_destroy failed");
    exit(EXIT_FAILURE);
  }
  if(pthread_barrier_destroy(&pool->ready) != 0){
    perror("pthread_barrier_destroy failed");
    exit(EXIT_FAILURE);
  }




  free(pool->args);
  free(pool->threads);
  list_free(pool->task_list);
  free(pool);

}