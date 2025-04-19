#ifndef VIEW_H
#define VIEW_H

typedef enum {
    PHENOMENA, //Rain, Clear, Clouds etc...
    TIME,      //hh:mm 24h
    MAIN,      //Main information (t_now, t_now_feels_like, max/min forecast)
    ERROR,     //On some error
    RECOVER,   //Error gone
} view_type;

typedef struct {
    char *text;
    view_type type;
} view_data_t;

void view_xHandler(void *queue);

#endif //VIEW_H
