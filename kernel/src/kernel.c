#include <include/kernel.h>

int main() {

    inicializar_config("kernel");    
    log_info(LOGGER_KERNEL, "Iniciando KERNEL\n");


    //Conectamos con memoria
    socket_kernel_memoria = crear_conexion(IP_MEMORIA, PUERTO_MEMORIA);
    log_info(LOGGER_KERNEL, "IP_MEMORIA: %s \nPUERTO_MEMORIA %s", IP_MEMORIA, PUERTO_MEMORIA);
    enviar_mensaje("Hola MEMORIA, soy Kernel", socket_kernel_memoria);
    paquete(socket_kernel_memoria, LOGGER_KERNEL);

    //Conectamos con dispatch
    socket_kernel_cpu_dispatch = crear_conexion(IP_CPU,PUERTO_CPU_DISPATCH);
    log_info(LOGGER_KERNEL, "IP_CPU: %s \nPUERTO_CPU_DISPATCH %s", IP_CPU, PUERTO_CPU_DISPATCH);
    enviar_mensaje("Hola CPU DISPATCH, soy KERNEL" , socket_kernel_cpu_dispatch);
    paquete(socket_kernel_cpu_dispatch , LOGGER_KERNEL);

    //Conectamos con interrupt
    socket_kernel_cpu_interrupt = crear_conexion(IP_CPU,PUERTO_CPU_INTERRUPT);
    log_info(LOGGER_KERNEL, "IP_CPU: %s \nPUERTO_CPU_INTERRUPT %s", IP_CPU, PUERTO_CPU_INTERRUPT);
    enviar_mensaje("Hola CPU INTERRUPT, soy KERNEL" , socket_kernel_cpu_interrupt);
    paquete(socket_kernel_cpu_interrupt , LOGGER_KERNEL);
    
    
    crear_primer_proceso();


    int sockets[] = {socket_kernel_memoria, socket_kernel_cpu_dispatch, socket_kernel_cpu_interrupt, -1};
    terminar_programa(CONFIG_KERNEL, LOGGER_KERNEL, sockets);

    return 0;
}


void inicializar_config(char* arg){
    /*
    char config_path[256];
    strcpy(config_path, arg);
    strcat(config_path, ".config");
    */
    char config_path[256];
    strcpy(config_path, "../");
    strcat(config_path, arg);
    strcat(config_path, ".config");

    LOGGER_KERNEL = iniciar_logger("kernel.log", "KERNEL");
    CONFIG_KERNEL = iniciar_config(config_path, "KERNEL");
    
    IP_MEMORIA = config_get_string_value(CONFIG_KERNEL, "IP_MEMORIA");
    PUERTO_MEMORIA = config_get_string_value(CONFIG_KERNEL, "PUERTO_MEMORIA");
    IP_CPU = config_get_string_value(CONFIG_KERNEL, "IP_CPU");
    PUERTO_CPU_DISPATCH = config_get_string_value(CONFIG_KERNEL, "PUERTO_CPU_DISPATCH");
    PUERTO_CPU_INTERRUPT = config_get_string_value(CONFIG_KERNEL, "PUERTO_CPU_INTERRUPT");
    ALGORITMO_PLANIFICACION = config_get_string_value(CONFIG_KERNEL, "ALGORITMO_PLANIFICACION");
    QUANTUM = config_get_string_value(CONFIG_KERNEL, "QUANTUM");
    LOG_LEVEL = config_get_string_value(CONFIG_KERNEL, "LOG_LEVEL");
}


t_pcb crear_proceso(char*);

t_pcb crear_proceso(char* path_proceso){
    
    int pid = asignar_pid();
    int* tids = asignar_tids();
    pthread_mutex_t* mutexs = asignar_mutexs();
    int prioridad = asignar_prioridad();
    t_pcb* pcb = crearPcb(pid, tids, NEW, mutexs, prioridad);

    pthread_mutex_lock(&pcbs_de_procesos_en_sistema_mutex);
    list_add(pcbs_de_procesos_en_sistema, pcb);
    pthread_mutex_unlock(&pcbs_de_procesos_en_sistema_mutex);

    log_info(LOGGER_KERNEL, "Proceso %d creado en NEW", pcb->PID);

    enviar_proceso_a_memoria(pcb->pid, path_proceso);

}