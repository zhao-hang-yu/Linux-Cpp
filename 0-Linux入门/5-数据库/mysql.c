#include <stdio.h>
#include <mysql/mysql.h>
#include <string.h>

#define DB_SERVER_IP    "192.168.100.131"
#define DB_SERVER_PORT  3306
#define DB_USERNAME     "admin"
#define DB_PASSWORD     "1234"
#define DB_DEFAULTDB    "DB_LEARN1"

#define SQL_INSERT_TBL_USER		"INSERT INTO TB_USER(U_NAME, U_GENDER) VALUES('King', 'man');"
#define SQL_SELECT_TBL_USER		"SELECT * FROM TB_USER;"
#define SQL_DELETE_TBL_USER		"CALL PROC_DELETE_USER('King');"

#define SQL_INSERT_IMG_USER		"INSERT TB_USER(U_NAME, U_GENDER, U_IMG) VALUES('King', 'man', ?);"
#define SQL_SELECT_IMG_USER		"SELECT U_IMG FROM TB_USER WHERE U_NAME='King';"
#define FILE_IMAGE_LENGTH		(64*1024)

int mysql_select(MYSQL *mysql) 
{
    // SQL
    if (mysql_real_query(mysql, SQL_SELECT_TBL_USER, strlen(SQL_SELECT_TBL_USER)))
    {
        printf("mysql_real_query error: %s\n", mysql_error(mysql));
        return -1;
    }
    // store
    MYSQL_RES *res = mysql_store_result(mysql);
    if (res == NULL)
    {
        printf("mysql_real_query error: %s\n", mysql_error(mysql));
        return -2;
    }
    // rows fields
    int rows = mysql_num_rows(res);
    int fields = mysql_num_fields(res);
    printf("rows: %d, fields: %d\n", rows, fields);
    // fetch
    MYSQL_ROW row;
    while ((row = mysql_fetch_row(res)))
    {
        for (int i = 0; i < fields; i++)
        {
            printf("%s\t", row[i]);
        }
        puts("");
    }
}

// node server & image
int read_image(char *filename, char *buffer)
{
    if (filename == NULL || buffer == NULL)
    {
        return -1;
    }
    FILE *fp = fopen(filename, "rb");
    if (fp == NULL)
    {
        printf("fopen failed\n");
        return -2;
    }
    fseek(fp, 0, SEEK_END); // 把指针放到最后
    int length = ftell(fp);
    fseek(fp, 0, SEEK_SET); // 把指针放到最前

    int size = fread(buffer, 1, length, fp); // 将图片数据读取 存储到buffer
    if (size != length)
    {
        printf("fread failed\n");
        return -3;
    }
    fclose(fp);
    return size;
}

int write_image(char *filename, char *buffer, int length)
{
    if (filename == NULL || buffer == NULL || length <= 0)
    {
        return -1;
    }
    FILE *fp = fopen(filename, "wb+"); // +代表如果没有这个文件 则创建
    if (fp == NULL)
    {
        printf("fopen failed\n");
        return -2;
    }

    int size = fwrite(buffer, 1, length, fp);
    if (size != length)
    {
        printf("fwrite failed\n");
        return -3;
    }
    fclose(fp);
    return size;
}


// node server & db server
int mysql_write(MYSQL *handle, char *buffer, int length)
{
    if (handle == NULL || buffer == NULL || length <= 0)
    {
        return -1;
    }
    // prepared stateme预处理语句
    MYSQL_STMT *stmt = mysql_stmt_init(handle);
    int ret = mysql_stmt_prepare(stmt, SQL_INSERT_IMG_USER, strlen(SQL_INSERT_IMG_USER));
    if (ret)
    {
        printf("mysql_stmt_prepare error: %s\n", mysql_error(handle));
        return -2;
    }
    MYSQL_BIND param = {0};
    param.buffer_type = MYSQL_TYPE_LONG_BLOB;
    param.buffer = NULL;
    param.length = NULL;
    param.is_null = 0;
    // 将参数绑定到预处理语句上
    ret = mysql_stmt_bind_param(stmt, &param);
    if (ret)
    {
        printf("mysql_stmt_bind_param error: %s\n", mysql_error(handle));
        return -3;
    }
    // 把大对象数据分批地发送给预处理语句中绑定的 BLOB 或 TEXT 参数
    ret = mysql_stmt_send_long_data(stmt, 0, buffer, length);
    if (ret)
    {
        printf("mysql_stmt_send_long_data error: %s\n", mysql_error(handle));
        return -4;
    }
    // 真正执行预处理语句
    ret = mysql_stmt_execute(stmt);
    if (ret)
    {
        printf("mysql_stmt_execute error: %s\n", mysql_error(handle));
        return -5;
    }
    ret = mysql_stmt_close(stmt);
    if (ret)
    {
        printf("mysql_stmt_close error: %s\n", mysql_error(handle));
        return -6;
    }
    return ret;
}

int mysql_read(MYSQL *handle, char *buffer, int length)
{
    if (handle == NULL || buffer == NULL || length <= 0)
    {
        return -1;
    }
    MYSQL_STMT *stmt = mysql_stmt_init(handle);
    int ret = mysql_stmt_prepare(stmt, SQL_SELECT_IMG_USER, strlen(SQL_SELECT_IMG_USER));
    if (ret)
    {
        printf("mysql_stmt_prepare error: %s\n", mysql_error(handle));
        return -2;
    }
    MYSQL_BIND result = {0};
    result.buffer_type  = MYSQL_TYPE_LONG_BLOB;
    unsigned long total_length = 0;
    result.length = &total_length;
    ret = mysql_stmt_bind_result(stmt, &result);
    if (ret)
    {
        printf("mysql_stmt_bind_result error: %s\n", mysql_error(handle));
        return -3;
    }
    ret = mysql_stmt_execute(stmt);
    if (ret)
    {
        printf("mysql_stmt_execute error: %s\n", mysql_error(handle));
        return -4;
    }
    // 从服务器端将结果一次性读取到本地内存
    ret = mysql_stmt_store_result(stmt);
    if (ret)
    {
        printf("mysql_store_result error: %s\n", mysql_error(handle));
        return -5;
    }
    // 查询结果有多条
    while (1)
    {
        ret = mysql_stmt_fetch(stmt);
        if (ret != 0 && ret != MYSQL_DATA_TRUNCATED) break; // 没取到或取了一半数据
        int start = 0;
        while (start < (int)total_length)
        {
            result.buffer = buffer + start; // result.buffer是一个指针，指向buffer + start，所以每次的数据都会写在buffer + start
			result.buffer_length = 1; // 每次写一个长度
			mysql_stmt_fetch_column(stmt, &result, 0, start);
			start += result.buffer_length;
        }
    }
    mysql_stmt_close(stmt);
	return total_length;
}

int main()
{
    MYSQL mysql;
    if (mysql_init(&mysql) == NULL)
    {
        printf("mysql_init error: %s", mysql_error(&mysql));
        return -1;
    }
    if (!mysql_real_connect(&mysql, DB_SERVER_IP, DB_USERNAME, DB_PASSWORD, DB_DEFAULTDB, DB_SERVER_PORT, NULL, 0))
    {
        printf("mysql_real_connect error: %s", mysql_error(&mysql));
        goto Exit;
    }

    mysql_select(&mysql);
    
#if 0
    printf("case : mysql --> insert \n");
    if (mysql_real_query(&mysql, SQL_INSERT_TBL_USER, strlen(SQL_INSERT_TBL_USER)))
    {
        printf("mysql_real_query error: %s\n", mysql_error(&mysql));
        goto Exit;
    }
#endif

#if 0
	printf("case : mysql --> delete \n");
    if (mysql_real_query(&mysql, SQL_DELETE_TBL_USER, strlen(SQL_DELETE_TBL_USER))) {
		printf("mysql_real_query : %s\n", mysql_error(&mysql));
		goto Exit;
	}
#endif
	mysql_select(&mysql);
#if 1
	printf("case : mysql --> read image and mysql write\n");
    char buffer[FILE_IMAGE_LENGTH] = {0};
    int length = read_image("0voice.jpg", buffer);
    if (length < 0) goto Exit;
    mysql_write(&mysql, buffer, length);

	printf("case : mysql --> mysql read and write image\n");
    memset(buffer, 0, FILE_IMAGE_LENGTH);
    length = mysql_read(&mysql, buffer, FILE_IMAGE_LENGTH);
    write_image("a.jpg", buffer, length);

#endif
    

Exit:
	mysql_close(&mysql);
    return 0;
}
