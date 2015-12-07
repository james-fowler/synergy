/*
 * Copyright (C) 2015 James Fowler
 */


extern "C" {

void				init(void* log, void* arch);
int				initEvent(void (*sendEvent)(const char*, void*));
void*			invoke(const char* command, void** args);
void				cleanup();

}

