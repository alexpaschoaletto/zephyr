#ifndef _APP_HELPER_H_
#define _APP_HELPER_H_

void report_cbs_settings(){
    printk("\n//////////////////////////////////////////////////////////////////////////////////////\n");
    printk("\nBoard:\t\t%s\n", CONFIG_BOARD_TARGET);
    #ifdef CONFIG_CBS
            printk("[CBS]\t\tCBS enabled.\n");
        #ifdef CONFIG_CBS_LOG
            printk("[CBS]\t\tCBS events logging: enabled.\n");
        #else
            printk("[CBS]\t\tCBS events logging: disabled.\n");
        #endif
        #ifdef CONFIG_TIMEOUT_64BIT
            printk("[CBS]\t\tSYS 64-bit timeouts: supported.\n");
        #else
            printk("[CBS]\t\tSYS 64-bit timeouts: not supported. using 32-bit API instead.\n");
        #endif
    #else
        printk("[CBS]\t\tCBS disabled.\n");
    #endif
    printk("\n//////////////////////////////////////////////////////////////////////////////////////\n\n");
}

#endif