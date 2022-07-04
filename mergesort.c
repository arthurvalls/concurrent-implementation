#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include "timer.h"

/* Para executar o programa é necessário fornecer o tamanho do vetor e a quantidade de threads que serão utilizadas,
por exemplo: ./mergesort 100000 4  
Para que o programa contabilize o tempo sequencial e concorrente, e consequentemente a aceleração, é necessário ter
o arquivo timer.h na mesma pasta desse programa, como disponibilizado no GitHub.*/

int dim, nthread; // dimensao do vetor e numero de threads

int bloco; // numero de elementos que cada thread lerá e ordenará utilizando merge sort; bloco * nthread deve resultar no tamanho "dim" do vetor

int desvio; // variavel responsavel por controlar o erro de divisoes nao exatas

int *vetSeq;  // vetor sequencial a ser ordenado
int *vetConc; // vetor concorrente a ser ordenado

// funcao merge padrao, utilizada para unir os vetores menores já ordenados
void merge(int *vetor, int comeco, int meio, int fim)
{
    int i, j, k; // variaveis contadoras
    int metade1 = meio - comeco + 1; // tamanho da primeira metade do vetor
    int metade2 = fim - meio; // tamanho da segunda metade do vetor

    // vetores temporarios
    int *E, *D;
    E = (int *)malloc(sizeof(int) * metade1); // vetor E corresponde a parte esquerda do vetor total
    D = (int *)malloc(sizeof(int) * metade2); // vetor D corresponde a parte direita do vetor total

    // copia as duas metades do vetor original para seus respectivos vetores temporarios
    for (i = 0; i < metade1; i++)
        E[i] = vetor[comeco + i];
    for (j = 0; j < metade2; j++)
        D[j] = vetor[meio + 1 + j];

    
    // juntar os subvetores
    i = 0;      // inicio do primeiro subvetor
    j = 0;      // inicio do segundo subvetor
    k = comeco; // inicio do vetor com "merge"
    while (i < metade1 && j < metade2)
    {
        if (E[i] <= D[j])
        {
            vetor[k] = E[i];
            i++;
        }
        else
        {
            vetor[k] = D[j];
            j++;
        }
        k++;
    }
    while (i < metade1)
    {
        vetor[k] = E[i];
        i++;
        k++;
    }
    while (j < metade2)
    {
        vetor[k] = D[j];
        j++;
        k++;
    }
    
    // libera os vetores temporarios
    free(E);
    free(D);
}

// mergesort recursivo padrao
void mergesort(int *vetor, int comeco, int fim)
{
    if (comeco < fim)
    {
        int meio = comeco + (fim - comeco) / 2;
        // ordena primeira e seguda metade do vetor
        mergesort(vetor, comeco, meio);
        mergesort(vetor, meio + 1, fim);
        // junta as duas partes
        merge(vetor, comeco, meio, fim);
    }
}

// funcao responsavel por juntar os subvetores ordenados por cada thread, ordenando o vetor original que ja possui sessoes ordenadas
void merge_global(int *vet, int nthread, int j)
{ // o valor inteiro "j" delimita o periodo do merge em questão.
    for (int i = 0; i < nthread; i = i + 2)
    {
        int comeco = i * (bloco * j); // j multiplica bloco, que é o número de elementos que cada thread irá ordenar
        int fim = ((i + 2) * bloco * j) - 1;
        int meio = comeco + (bloco * j) - 1;
        if (fim >= dim)
        {
            fim = dim - 1;
        }
        merge(vetConc, comeco, meio, fim);
    }
    if (nthread / 2 >= 1)
    { // recursivamente faz o merge com pares de subvetores ordenados inlusos em vetConc
        merge_global(vet, nthread / 2, j * 2);
    }
}

// teste de corretude
void teste_ordem(int *vet)
{
    for (int i = 1; i < nthread; i++)
    {
        if (vet[i] < vet[i - 1])
        {
            printf("Error. Fora de ordem: valor %d \n", vet[i]);
            return;
        }
    }
}

// teste de igualdade entre os vetores sequencial e concorrente
void teste_igualdade(int *vet1, int *vet2)
{
    for (int i = 0; i < dim; i++)
    {
        if (vet1[i] != vet2[i])
        {
            printf("ERRO--Vetores diferentes");
            return;
        }
    }
}

// funcao que a thread executa
void *mergesort_thread(void *arg)
{
    int id = (long)arg;
    int comeco = id * (bloco);
    int fim = (id + 1) * (bloco)-1;
    if (id == nthread - 1)
    {
        fim += desvio; // compensacao pela divisao nao exata
    }
    int meio = comeco + (fim - comeco) / 2;
    if (comeco < fim)
    {
        mergesort(vetConc, comeco, fim);
        mergesort(vetConc, comeco + 1, fim);
        merge(vetConc, comeco, meio, fim);
    }
    return NULL;
}

// funcao principal
int main(int argc, const char *argv[])
{
    double inicio, fim, tempSeq, tempConc;
    if (argc < 3)
    {
        fprintf(stderr, "Digite: %s <dimensao do vetor> <numero threads>\n", argv[0]);
        return 3;
    }
    dim = atoll(argv[1]);
    nthread = atoi(argv[2]);
    
    bloco = dim / nthread; 
    desvio = dim % nthread; // resto da divisao

    // aloca os vetores
    vetConc = (int *)malloc(sizeof(int) * dim);
    vetSeq = (int *)malloc(sizeof(int) * dim);

    // inicializa vetor com valores aleatorios entre 0 e 100
    srand(time(NULL));
    for (int i = 0; i < dim; i++)
    {
        vetSeq[i] = rand() % 100;
        vetConc[i] = vetSeq[i];
    }

    // execucao sequencial
    GET_TIME(inicio);
    mergesort(vetSeq, 0, dim - 1);

    teste_ordem(vetSeq);

    // tempo sequencial
    GET_TIME(fim);
    tempSeq = fim - inicio;
    printf("Tempo sequencial: %lf\n", tempSeq);

    // execucao concorrente
    GET_TIME(inicio);

    // cria as threads
    pthread_t threads[nthread];
    for (long i = 0; i < nthread; i++)
    {
        if (pthread_create(&threads[i], NULL, mergesort_thread, (void *)i))
        {
            puts("ERRO--pthread_create");
            return 3;
        }
    }

    // espera as threads terminarem
    for (long i = 0; i < nthread; i++)
    {
        pthread_join(threads[i], NULL);
    }

    // junta vetores ordenados locais das threads
    merge_global(vetConc, nthread, 1);

    // tempo concorrente
    GET_TIME(fim);
    tempConc = fim - inicio;
    printf("Tempo concorrente: %lf\n", tempConc);

    // aceleracao
    printf("Aceleração: %lf\n", tempSeq / tempConc);

    // teste de corretude
    teste_ordem(vetConc);
    teste_igualdade(vetSeq, vetConc);

    // libera os vetores
    free(vetConc);
    free(vetSeq);
}
