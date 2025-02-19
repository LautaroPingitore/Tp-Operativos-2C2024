#include "include/gestor.h"

char* PUERTO_ESCUCHA;
char* IP_FILESYSTEM;
char* PUERTO_FILESYSTEM;
int TAM_MEMORIA;
char* PATH_INSTRUCCIONES;
int RETARDO_RESPUESTA;
char* ESQUEMA;
char* ALGORITMO_BUSQUEDA;
char** PARTICIONES_FIJAS;
char* LOG_LEVEL; 
char* IP_MEMORIA;

int socket_memoria;
int socket_memoria_kernel;
int socket_memoria_cpu;
int socket_memoria_filesystem;

t_log* LOGGER_MEMORIA;
t_config* CONFIG_MEMORIA;

pthread_t hilo_server_memoria;
pthread_t hilo_filesystem;

bool mensaje_okey = false;

int main(int argc, char* argv[]) {
    if(argc != 2) {
        printf("Uso: %s [archivo_config] \n", argv[0]);
        return -1;
    }

    char* config = argv[1];

    inicializar_config(config);
    inicializar_programa();

    pthread_exit(NULL); // Evita que el hilo principal finalice y permite que los hilos creados sigan ejecutándose

    return 0;
}

void inicializar_programa() {    
    // Inicializar listas de estructuras de procesos y memoria
    inicializar_datos();
    
    // Inicializar particiones de memoria en base al esquema (fijo o dinámico)
	t_list* particiones_fijas = obtener_particiones_fijas(PARTICIONES_FIJAS);
    inicializar_lista_particiones(ESQUEMA, particiones_fijas);

    list_destroy_and_destroy_elements(particiones_fijas, free);

    inicializar_memoria_sistema();

    // Configurar conexiones de memoria a otros módulos
    log_info(LOGGER_MEMORIA, "Inicializando servidor de memoria en el puerto %s", PUERTO_ESCUCHA);
//    socket_memoria = iniciar_servidor(PUERTO_ESCUCHA, LOGGER_MEMORIA, IP_MEMORIA, "MEMORIA");

    iniciar_conexiones();
    
}

void inicializar_config(char *arg) {
	
	char config_path[256];
    strcpy(config_path, "../configs/");
    strcat(config_path, arg);
    strcat(config_path, ".config");

	LOGGER_MEMORIA = iniciar_logger("memoria.log", "Servidor Memoria");
	CONFIG_MEMORIA = iniciar_config(config_path, "MEMORIA");

	PUERTO_ESCUCHA = config_get_string_value(CONFIG_MEMORIA, "PUERTO_ESCUCHA");
    IP_FILESYSTEM = config_get_string_value(CONFIG_MEMORIA,"IP_FILESYSTEM");
    PUERTO_FILESYSTEM = config_get_string_value(CONFIG_MEMORIA,"PUERTO_FILESYSTEM");
	TAM_MEMORIA = config_get_int_value(CONFIG_MEMORIA, "TAM_MEMORIA");
	PATH_INSTRUCCIONES = config_get_string_value(CONFIG_MEMORIA, "PATH_INSTRUCCIONES");
	RETARDO_RESPUESTA = config_get_int_value(CONFIG_MEMORIA, "RETARDO_RESPUESTA");
    ESQUEMA = config_get_string_value(CONFIG_MEMORIA,"ESQUEMA");
	ALGORITMO_BUSQUEDA = config_get_string_value(CONFIG_MEMORIA, "ALGORITMO_BUSQUEDA");
	PARTICIONES_FIJAS = config_get_array_value(CONFIG_MEMORIA, "PARTICIONES");
    LOG_LEVEL = config_get_string_value(CONFIG_MEMORIA,"LOG_LEVEL");

	IP_MEMORIA = config_get_string_value(CONFIG_MEMORIA,"IP_MEMORIA");
}

t_list* obtener_particiones_fijas(char** particiones) {
    t_list* lista_particiones_fijas = list_create();
    for (int i = 0; particiones[i] != NULL; i++) {
        int* tam_particion = malloc(sizeof(int));
        *tam_particion = atoi(particiones[i]);
        list_add(lista_particiones_fijas, tam_particion);
    }
    return lista_particiones_fijas;
}

void iniciar_conexiones() {
    // Iniciar servidor Memoria
    socket_memoria = iniciar_servidor(PUERTO_ESCUCHA, LOGGER_MEMORIA, IP_MEMORIA, "MEMORIA");
    if (socket_memoria == -1) {
        log_error(LOGGER_MEMORIA, "No se pudo iniciar el servidor de Memoria.");
        exit(EXIT_FAILURE);
    }
    log_info(LOGGER_MEMORIA, "Servidor memoria iniciado y escuchando en el puerto %s", PUERTO_ESCUCHA);

    // Conexión cliente FileSystem
    socket_memoria_filesystem = crear_conexion(IP_FILESYSTEM, PUERTO_FILESYSTEM);
    if (socket_memoria_filesystem < 0) {
        log_error(LOGGER_MEMORIA, "No se pudo conectar con el módulo FileSystem");
        exit(EXIT_FAILURE);
    }
    enviar_handshake(socket_memoria_filesystem, HANDSHAKE_memoria);
    log_info(LOGGER_MEMORIA, "Conexión con FileSystem establecida exitosamente, se envio codigo %d", HANDSHAKE_memoria);
    pthread_create(&hilo_filesystem, NULL, manejar_comunicacion_filesystem, NULL);
    pthread_detach(hilo_filesystem);

    // Iniciar hilo servidor
    pthread_create(&hilo_server_memoria, NULL, escuchar_memoria, NULL);
    pthread_detach(hilo_server_memoria);

}

void* escuchar_memoria() 
{
    log_info(LOGGER_MEMORIA, "El hilo de escuchar_memoria ha iniciado.");
    while (server_escuchar(LOGGER_MEMORIA, "MEMORIA", socket_memoria)) {
        log_info(LOGGER_MEMORIA, "Conexión procesada.");
    }
    log_warning(LOGGER_MEMORIA, "El servidor de memoria terminó inesperadamente.");
    return NULL;
}

int server_escuchar(t_log *logger, char *server_name, int server_socket) {
    while (1) {
        int cliente_socket = esperar_cliente(server_socket, logger);

        if (cliente_socket == -1) {
            log_warning(logger, "[%s] Error al aceptar conexión. Reintentando...", server_name);
            continue;
        }

        pthread_t hilo_cliente;
        t_procesar_conexion_args *args = malloc(sizeof(t_procesar_conexion_args));
        if (!args) {
            log_error(logger, "[%s] Error al asignar memoria para las conexiones.", server_name);
            close(cliente_socket);
            continue;
        }

        args->log = logger;
        args->fd = cliente_socket;
        args->server_name = strdup(server_name);

        if (pthread_create(&hilo_cliente, NULL, procesar_conexion_memoria, (void*)args) != 0) {
            log_error(logger, "[%s] Error al crear hilo para cliente.", server_name);
            
            free(args);
            close(cliente_socket);
            continue;
        }

        pthread_detach(hilo_cliente);
        log_info(logger, "[%s] Hilo creado para procesar la conexión.", server_name);
    }
    return 0; // Este punto no se alcanza debido al ciclo infinito.
}

void* manejar_comunicacion_filesystem(void* args) {
    procesar_conexion_filesystem(socket_memoria_filesystem, "FILESYSTEM");
    return NULL;
}

void procesar_conexion_filesystem(int cliente_socket, const char* modulo) {
    op_code cod;
    while (cliente_socket != -1) {
        // Recibir código de operación
        ssize_t bytes_recibidos = recv(cliente_socket, &cod, sizeof(op_code), MSG_WAITALL);
        if (bytes_recibidos != sizeof(op_code) || bytes_recibidos == 0) {
            log_error(LOGGER_MEMORIA, "Cliente desconectado");
            break;
        }

        usleep(RETARDO_RESPUESTA * 1000);

        switch (cod) {
        case MENSAJE:
            char* respuesta = recibir_mensaje(cliente_socket);
            if (respuesta && strcmp(respuesta, "OK") == 0) {
                mensaje_okey = true;
            } else {
                mensaje_okey = false;
            }
            sem_post(&sem_respuesta);
            free(respuesta);
            break;
        
        default:
            log_error(LOGGER_MEMORIA, "Codigo desconocido en el servidor de %s, Cod OP: %d", modulo, cod);
            break;
        }
    }
}

void* procesar_conexion_memoria(void *void_args){
    t_procesar_conexion_args *args = (t_procesar_conexion_args *)void_args;
    //t_log *logger = args->log;
    int cliente_socket = args->fd;
    char *server_name = args->server_name;

    free(args);

    op_code cod;
    while (cliente_socket != -1) {
        // Recibir código de operación
        ssize_t bytes_recibidos = recv(cliente_socket, &cod, sizeof(op_code), MSG_WAITALL);
        if (bytes_recibidos != sizeof(op_code) || bytes_recibidos == 0) {
            log_error(LOGGER_MEMORIA, "Cliente desconectado");
            terminar_memoria();
            break;
        }

        usleep(RETARDO_RESPUESTA * 1000);

        switch (cod) {
            case HANDSHAKE_kernel: // Simplemente avisa que se conecta a kernel 
                log_info(LOGGER_MEMORIA, "## KERNEL Conectado - FD del socket: <%d>", cliente_socket);
                break;

            case PROCESS_CREATE:
                t_proceso_memoria* proceso_nuevo = recibir_proceso(cliente_socket);
                if(proceso_nuevo == NULL) {
                    enviar_mensaje("ERROR",cliente_socket);
                    break;
                }
                log_info(LOGGER_MEMORIA, "Se recibio el proceso a crear %d", proceso_nuevo->pid);
                
                if (asignar_espacio_memoria(proceso_nuevo, ALGORITMO_BUSQUEDA) == -1) {
                    log_warning(LOGGER_MEMORIA, "NO HAY ESPACIO");
                    enviar_mensaje("ERROR", cliente_socket);
                    free(proceso_nuevo->contexto);
                    free(proceso_nuevo);
                } else {
                    pthread_mutex_lock(&mutex_procesos);
                    list_add(lista_procesos, proceso_nuevo);
                    pthread_mutex_unlock(&mutex_procesos);
                    enviar_mensaje("OK", cliente_socket);
                    log_info(LOGGER_MEMORIA, "## Proceso <Creado> -  PID: <%d> - Tamaño: <%d>", proceso_nuevo->pid, proceso_nuevo->limite);
                }
                break ;

            case PROCESS_EXIT:
                t_proceso_memoria* proceso_a_eliminar = recibir_proceso(cliente_socket);

                if (proceso_a_eliminar == NULL) {
                    enviar_mensaje("ERROR", cliente_socket);
                    break;
                }
            
                if(liberar_espacio_memoria(proceso_a_eliminar->pid) == 1) {
                    pthread_mutex_lock(&mutex_procesos);
                    eliminar_proceso_de_lista(proceso_a_eliminar->pid);
                    pthread_mutex_unlock(&mutex_procesos);

                    log_info(LOGGER_MEMORIA, "## Proceso <Destruido> -  PID: <%d> - Tamaño: <%d>", proceso_a_eliminar->pid, proceso_a_eliminar->limite);
                    enviar_mensaje("OK", cliente_socket);
                } else {
                    enviar_mensaje("ERROR", cliente_socket);
                }
                free(proceso_a_eliminar->contexto);
                free(proceso_a_eliminar);
                break;

            case THREAD_CREATE:
                t_hilo_memoria* hilo_a_crear = recibir_hilo_memoria(cliente_socket);
                if(!hilo_a_crear) {
                    enviar_mensaje("ERROR", cliente_socket);
                    break;
                }
                agregar_instrucciones_a_lista(hilo_a_crear->pid_padre, hilo_a_crear->tid, hilo_a_crear->archivo);
                log_info(LOGGER_MEMORIA, "## Hilo <Creado> - (<%d>:<%d>)", hilo_a_crear->pid_padre, hilo_a_crear->tid);
                break;

            case THREAD_EXIT:
                t_hilo_memoria* hilo_a_eliminar = recibir_hilo_memoria(cliente_socket);
                if(!hilo_a_eliminar) {
                    enviar_mensaje("ERROR", cliente_socket);
                    break;
                }

                log_info(LOGGER_MEMORIA, "## Hilo <Destruido> - (<%d>:<%d>)", hilo_a_eliminar->pid_padre, hilo_a_eliminar->tid);

                eliminar_espacio_hilo(hilo_a_eliminar);
                break;

            case DUMP_MEMORY:
                t_pid_tid* ident_dm = recibir_identificadores(cliente_socket);
                if(!ident_dm) {
                    enviar_mensaje("ERROR", cliente_socket);
                } 
                
                if(solicitar_archivo_filesystem(ident_dm->pid, ident_dm->tid) == 0) {
                    log_info(LOGGER_MEMORIA, "## Memory Dump solicitado - (<%d>:<%d>)",
                             ident_dm->pid, ident_dm->tid);
                } else {
                    enviar_mensaje("ERROR", cliente_socket);
                }

                sem_wait(&sem_respuesta);

                if(mensaje_okey) {
                    enviar_mensaje("OK", cliente_socket);
                } else {
                    enviar_mensaje("ERROR", cliente_socket);
                }

                break;

            case HANDSHAKE_cpu: //AVISA QUE SE CONECTO A CPU
                log_info(LOGGER_MEMORIA, "## CPU Conectado - FD del socket: <%d>", cliente_socket);
                enviar_handshake(cliente_socket, HANDSHAKE_memoria);
                break;

            case CONTEXTO:
                t_pid_tid* ident_cx = recibir_identificadores(cliente_socket);
                procesar_solicitud_contexto(cliente_socket, ident_cx->pid, ident_cx->tid);
                free(ident_cx);
                break;

            case ACTUALIZAR_CONTEXTO:
                t_actualizar_contexto* act_cont = recibir_actualizacion(cliente_socket);
                procesar_actualizacion_contexto(cliente_socket, act_cont->pid, act_cont->tid, act_cont->contexto);
                free(act_cont->contexto);
                free(act_cont);
                break;

            case PEDIDO_WRITE_MEM:
                t_write_mem* wri_mem = recibir_write_mem(cliente_socket);
                int resultado = escribir_memoria(wri_mem->dire_fisica_wm, wri_mem->valor_escribido);
                if(resultado == 1) {
                    enviar_mensaje("SE PUDO ESCRIBIR", cliente_socket);
                } else {
                    enviar_mensaje("ERROR", cliente_socket);
                }
                uint32_t tam_valor_escribido = sizeof(wri_mem->valor_escribido);
                log_info(LOGGER_MEMORIA, "## <Escritura> - (<%d>:<%d>) - Dir. Física: <%d> - Tamaño: <%d>",
                         wri_mem->pid, wri_mem->tid, wri_mem->dire_fisica_wm, tam_valor_escribido);
                free(wri_mem);
                break;

            case PEDIDO_READ_MEM:
                t_read_mem* read_mem = recibir_read_mem(cliente_socket);
                uint32_t valor_leido = leer_memoria(read_mem->direccion_fisica);
                //log_warning(LOGGER_MEMORIA, "SE LEYO %d EN %d", valor_leido, read_mem->direccion_fisica);
           
                if (valor_leido != -1) {
                    enviar_valor_leido_cpu(cliente_socket, valor_leido);
                    uint32_t tam = sizeof(valor_leido);
                    log_info(LOGGER_MEMORIA, "## <Lectura> - (<%d>:<%d>) - Dir. Física: <%d> - Tamaño: <%d>",
                             read_mem->pid, read_mem->tid, read_mem->direccion_fisica, tam);
                } else {
                    enviar_mensaje("SEGMENTATION_FAULT", cliente_socket);
                }
                free(read_mem);
                break;

            case PEDIDO_INSTRUCCION:
                t_pedido_instruccion* ped_inst = recibir_pedido_instruccion(cliente_socket);

                t_instruccion* inst = obtener_instruccion(ped_inst->pid,ped_inst->tid, ped_inst->pc);

                if(enviar_instruccion(cliente_socket, inst) == 0) {
                    if(inst->parametro3 == -1) {
                        log_info(LOGGER_MEMORIA, "## Obtener instrucción - (<%d>:<%d>) - Instrucción: %s %s %s",
                                ped_inst->pid, ped_inst->tid, inst->nombre, inst->parametro1, inst->parametro2);
                    } else {
                        log_info(LOGGER_MEMORIA, "## Obtener instrucción - (<%d>:<%d>) - Instrucción: %s %s %s %d",
                                ped_inst->pid, ped_inst->tid, inst->nombre, inst->parametro1, inst->parametro2, inst->parametro3);
                    }
                } else {
                    enviar_mensaje("NO_INSTRUCCION", cliente_socket);
                }
                free(ped_inst);
                break;

            case SOLICITUD_BASE_MEMORIA:
                t_pedido_cpu* pedido_base = recibir_pedido_cpu(cliente_socket);
                uint32_t base = buscar_base_registro(pedido_base->pid, pedido_base->dire_reg);
                if (base == -1) {
                    log_error(LOGGER_MEMORIA, "No se pudo encontrar la base para el PID %d y registro %d", 
                            pedido_base->pid, pedido_base->dire_reg);
                    break;
                }
                int resultado_bm = enviar_valor_uint_cpu(cliente_socket, base, SOLICITUD_BASE_MEMORIA);
                if(resultado_bm == 0) {
                    log_info(LOGGER_MEMORIA, "Base enviada");
                } else {
                    log_error(LOGGER_MEMORIA, "Error al enviar el paquete");
                }
                break;

            case SOLICITUD_LIMITE_MEMORIA:
                t_pedido_cpu* pedido_limite = recibir_pedido_cpu(cliente_socket);
                free(pedido_limite);
                int resultado_lm = enviar_valor_uint_cpu(cliente_socket, sizeof(uint32_t), SOLICITUD_LIMITE_MEMORIA);
                if(resultado_lm == 0) {
                    log_info(LOGGER_MEMORIA, "Limite enviado");
                } else {
                    log_error(LOGGER_MEMORIA, "Error al enviar el paquete");
                }
                break;

            default:
                log_error(LOGGER_MEMORIA, "Codigo desconocido en el servidor de %s, Cod OP: %d", server_name, cod);
                break;
        }
    }


    log_warning(LOGGER_MEMORIA, "Finalizando conexión con el cliente.");
    close(cliente_socket); // Cerrar el socket del cliente
    free(args->server_name);

    return NULL;
}

void terminar_memoria() {
    free(memoria_sistema);
    list_destroy(lista_particiones);
    destruir_listas();
    destruir_mutexs();

    int sockets[] = {socket_memoria, socket_memoria_filesystem, -1};
    terminar_programa(CONFIG_MEMORIA, LOGGER_MEMORIA, sockets);
    exit(SUCCES);
}

void destruir_listas() {
    for(int i=0; i < list_size(lista_procesos); i++) {
        t_proceso_memoria* pACt = list_remove(lista_procesos, i);
        i--;
        free(pACt->contexto);
        free(pACt);
    }
    list_destroy(lista_procesos);

    for(int j=0; j < list_size(lista_instrucciones); j++) {
        t_hilo_instrucciones* hAct = list_remove(lista_instrucciones, j);
        j--;
        for(int k=0; k < list_size(hAct->instrucciones); k++) {
            t_instruccion* inst = list_remove(hAct->instrucciones, k);
            k--;
            free(inst->nombre);
            free(inst->parametro1);
            free(inst->parametro2);
            free(inst);
        }
        list_destroy(hAct->instrucciones);
        free(hAct);
    }
    list_destroy(lista_instrucciones);
}

void destruir_mutexs() {
    pthread_mutex_destroy(&mutex_procesos);
    pthread_mutex_destroy(&mutex_instrucciones);
}