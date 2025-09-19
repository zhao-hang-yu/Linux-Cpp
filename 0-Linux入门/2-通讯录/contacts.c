#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#define NAME_LENGTH     16
#define PHONE_LENGTH    32
#define BUFFER_LENGTH   128

#define INFO printf

// 使用do while使宏用起来更像一条语句
// define只能定义单行宏，\的作用是把多行宏定义连接成一行处理
#define LIST_INSERT(item, list) do {    \
    item->prev = NULL;                  \
    item->next = list;                  \
    if ((list) != NULL)                 \
        (list)->prev = item;            \
    (list) = item;                      \
} while(0)

#define LIST_REMOVE(item, list) do {    \
    if (item->prev != NULL)             \
        item->prev->next = item->next;  \
    if (item->next != NULL)             \
        item->next->prev = item->prev;  \
    if (list == item)                   \
        list = item->next;              \
    item->prev = item->next = NULL;     \
} while(0)


struct person
{
    /* data */
    char name[NAME_LENGTH];
    char phone[PHONE_LENGTH];

    struct person *next;
    struct person *prev;
};

struct contacts
{
    struct person *people;
    int count;
};

enum
{
    OPER_INSERT = 1, // 后面的会自动增加
    OPER_PRINT,
    OPER_DELETE,
    OPER_SEARCH,
    OPER_SAVE,
    OPER_LOAD
};

// interface
// 要修改里面的内容，使用二级指针
int person_insert(struct person **ppeople, struct person *ps) 
{
    if (ps == NULL) return -1;
    LIST_INSERT(ps, *ppeople);
    return 0;
}

int person_delete(struct person **people, struct person *ps) 
{
    if (ps == NULL) return -1;
    if (people == NULL) return -2;
    LIST_REMOVE(ps, *people);
    return 0;
}

struct person* person_search(struct person *people, char *name)
{
    struct person *item = NULL;
    for (item = people; item != NULL; item = item->next) 
        if (!strcmp(name, item->name)) 
            break;
    return item;
}

int person_traversal(struct person *people)
{
    struct person *item = NULL;
    for (item = people; item != NULL; item = item->next) 
        INFO("name:%s, phone:%s\n", item->name, item->phone);
    return 0;
}

int save_file(struct person *people, const char *filename)
{
    FILE *fp = fopen(filename, "w");
    if (fp == NULL) return -1;
    struct person *item = NULL;
    for (item = people; item != NULL; item = item->next)
    {
        fprintf(fp, "name:%s, phone:%s\n", item->name, item->phone);
        fflush(fp);
    }
    fclose(fp);
    return 0;
}

int parser_token(char *buffer, int length, char *name, char *phone)
{
    if (buffer == NULL) return -1;
    if (length == 0) return -2;

    int i = 0, j = 0, status = 0;
    for (i = 0; buffer[i] != ','; i++)
    {
        if (buffer[i] == ':')
        {
            status = 1;
        }
        else if (status == 1) 
        {
            name[j ++] = buffer[i];
        }
    }

    j = 0, status = 0;
    for (; i < length && buffer[i] != '\n'; i++) 
    {
        if (buffer[i] == ':')
        {
            status = 1;
        }
        else if (status == 1) 
        {
            phone[j ++] = buffer[i];
        }
    }

    INFO("file token: %s ---> %s\n", name, phone);
    return 0;
}

int load_file(struct person **ppeople, int *count, const char *filename) 
{
    FILE *fp = fopen(filename, "r");
    if (fp == NULL) return -1;
    struct person *item = NULL;
    while (!feof(fp))
    {
        char buffer[BUFFER_LENGTH] = {0};
        fgets(buffer, BUFFER_LENGTH, fp);
        // name:xxx, phone:xxx
        char name[NAME_LENGTH] = {0}, phone[PHONE_LENGTH] = {0};
        if (parser_token(buffer, strlen(buffer), name, phone) != 0)
            continue;
        // 存入链表
        struct person *p = (struct person*)malloc(sizeof(struct person));
        if (p == NULL)
            return -2;
        memcpy(p->name, name, NAME_LENGTH);
        memcpy(p->phone, phone, PHONE_LENGTH);
        person_insert(ppeople, p);
        (*count) ++;
    }
    return 0;
}


int insert_entry(struct contacts *cts)
{
    if (cts == NULL) return -1;
    struct person *p = (struct person*)malloc(sizeof(struct person));
    if (p == NULL) return -2;
    // name
    INFO("Please Input Name: \n");
    scanf("%s", p->name);
    // phone
    INFO("Please Input Phone: \n");
    scanf("%s", p->phone);
    // add
    if (0 != person_insert(&cts->people, p))
    {
        free(p);
        return -3;
    }
    cts->count ++;
    INFO("Insert Success\n");
}

int print_entry(struct contacts *cts)
{
    if (cts == NULL) return -1;
    person_traversal(cts->people);
    return 0;
}

int delete_entry(struct contacts *cts)
{
    if (cts == NULL) return -1;
    INFO("Please Input Name: \n");
    char name[NAME_LENGTH] = {0};
    scanf("%s", name);
    // person
    struct person *ps = person_search(cts->people, name);
    if (ps == NULL) 
    {
        INFO("Person don't Exit\n");
        return -2;
    }
    // delete
    person_delete(&cts->people, ps);
    free(ps);
    INFO("Delete Success\n");
    return 0;
}

int search_entry(struct contacts *cts)
{
    if (cts == NULL) return -1;
    INFO("Please Input Name: \n");
    char name[NAME_LENGTH] = {0};
    scanf("%s", name);
    // person
    struct person *ps = person_search(cts->people, name);
    if (ps == NULL) 
    {
        INFO("Person don't Exit\n");
        return -2;
    }
    // info
    INFO("name:%s, phone:%s\n", ps->name, ps->phone);
    return 0;
}

int save_entry(struct contacts *cts)
{
    if (cts == NULL) return -1;
    INFO("Please Input Save Filename: \n");
    char filename[NAME_LENGTH] = {0};
    scanf("%s", filename);
    save_file(cts->people, filename);
    INFO("Save Success!\n");
    return 0;
}

int laod_entry(struct contacts *cts)
{
    if (cts == NULL) return -1;
    INFO("Please Input Load Filename: \n");
    char filename[NAME_LENGTH] = {0};
    scanf("%s", filename);
    load_file(&cts->people, &cts->count, filename);
    INFO("Load Success!\n");
    return 0;
}

void menu_info() 
{
    INFO("\n\n********************************************************\n");
	INFO("***** 1. Add Person\t\t2. Print People ********\n");
	INFO("***** 3. Del Person\t\t4. Search Person *******\n");
	INFO("***** 5. Save People\t\t6. Load People *********\n");
	INFO("***** Other Key for Exiting Program ********************\n");
	INFO("********************************************************\n\n");
}

int main()
{
    struct contacts *cts = (struct contacts *)malloc(sizeof(struct contacts));
    if (cts == NULL) return -1;
    memset(cts, 0, sizeof(struct contacts));
    while (1) 
    {
        menu_info();
        int select = 0;
        scanf("%d", &select);
        switch (select)
        {
            case OPER_INSERT:
                insert_entry(cts);
                break;
            case OPER_PRINT:
                print_entry(cts);
                break;
            case OPER_DELETE:
                delete_entry(cts);
                break;
            case OPER_SEARCH:
                search_entry(cts);
                break;
            case OPER_SAVE:
                save_entry(cts);
                break;
            case OPER_LOAD:
                laod_entry(cts);
                break;
            default:
                goto exit;
        }
    }
    exit:
    return 0;
}