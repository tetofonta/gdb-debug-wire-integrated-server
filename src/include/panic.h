//
// Created by stefano on 03/08/22.
//

#ifndef ARDWINO_PANIC_H
#define ARDWINO_PANIC_H

#define MUST_SUCCEED(f, ret) do{ if(ret != f) panic(); }while(0)

__attribute__((noreturn)) void panic(void);

#endif //ARDWINO_PANIC_H
