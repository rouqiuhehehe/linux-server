//
// Created by Administrator on 2023/3/17.
//

#ifndef LINUX_SERVER_SRC_2_INTERNET_MIMETYPE_H_
#define LINUX_SERVER_SRC_2_INTERNET_MIMETYPE_H_

#define TYPE_HTML "text/html"
#define TYPE_PNG "image/png"
#define TYPE_PDF "application/pdf"

#define GET 1
#define POST 2

#define HTTP_OK 200
#define HTTP_Multiple 301
#define HTTP_NOTFOUND 404

int getFileMimetype (char *contentType, const char *ext)
{
    if (!strcmp(ext, "html"))
        strcpy(contentType, TYPE_HTML);
    if (!strcmp(ext, "png"))
        strcpy(contentType, TYPE_PNG);
    if (!strcmp(ext, "pdf"))
        strcpy(contentType, TYPE_PDF);

    return 0;
}
int resTypeIsFile (const char *contentType)
{
    if (!strcmp(contentType, TYPE_HTML)
        || !strcmp(contentType, TYPE_PNG)
        || !strcmp(contentType, TYPE_PDF))
        return 0;
    return -1;
}

int getHttpHeadState (int state, char *stateStr)
{
    if (stateStr == NULL || state < 100)
        return -1;
    if (state == HTTP_OK)
        strcpy(stateStr, "200 OK");
    else if (state == HTTP_Multiple)
        strcpy(stateStr, "301 Multiple Choices");
    else if (state == HTTP_NOTFOUND)
        strcpy(stateStr, "404 NOT FOUND");

    return 0;
}

#endif //LINUX_SERVER_SRC_2_INTERNET_MIMETYPE_H_
