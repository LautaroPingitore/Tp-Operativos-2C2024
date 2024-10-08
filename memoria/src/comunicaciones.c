#include "include/comunicaciones.h"

static void procesar_conexion_memoria(void *void_args)
{
    t_procesar_conexion_args *args = (t_procesar_conexion_args *)void_args;
    t_log *logger = args->log;
    int cliente_socket = args->fd;
    char *server_name = args->server_name;
    free(args);

    op_code cop;
    while (cliente_socket != -1)
    {
        if (recv(cliente_socket, &cop, sizeof(op_code), 0) != sizeof(op_code))
        {
            log_debug(logger, "Cliente desconectado.\n");
            return;
        }

        switch (cop)
        {
        // ----------------------
        // -- KERNEL - MEMORIA --
        // ----------------------
        case HANDSHAKE_kernel:
            recibir_mensaje(cliente_socket, logger);
            log_info(logger, "Este deberia ser el canal mediante el cual nos comunicamos con el KERNEL");
            break;

        case INICIALIZAR_PROCESO:
            // Aquí se inicializa el proceso y se crea su contexto de ejecución
            // uint32_t pid, base, limite;
            // recibir_inicializar_proceso(&pid, &base, &limite, cliente_socket);
            // crear_proceso(pid, base, limite); // Crear proceso y su contexto
            // log_info(logger, "Proceso inicializado con PID: %d", pid);
            log_info(logger, "No implementado. Respondiendo OK.");
            break;

        case FINALIZAR_PROCESO:
            // uint32_t pid_a_finalizar;
            // recibir_finalizar_proceso(&pid_a_finalizar, cliente_socket);
            // liberar_proceso(pid_a_finalizar);
            // log_info(logger, "Proceso con PID: %d finalizado", pid_a_finalizar);
            log_info(logger, "No implementado. Respondiendo OK.");
            break;

        // -------------------
        // -- CPU - MEMORIA --
        // -------------------
        case HANDSHAKE_cpu:
            recibir_mensaje(cliente_socket, logger);
            log_info(logger, "Este deberia ser el canal mediante el cual nos comunicamos con la CPU");
            break;

        case PEDIDO_INSTRUCCION:
            uint32_t pid, pc;
            recibir_pedido_instruccion(&pid, &pc, cliente_socket);
            t_instruccion *instruccion = obtener_instruccion(pid, pc);
            if (instruccion != NULL)
            {
                enviar_instruccion(cliente_socket, instruccion);
                log_info(logger, "Se envió la instrucción al PID %d, PC %d", pid, pc);
            }
            else
            {
                log_error(logger, "No se encontró la instrucción para PID %d, PC %d", pid, pc);
            }
            break;

        // Peticiones stub (sin hacer nada)
        case PEDIDO_SET:
            log_info(logger, "Peticiones de SET no implementadas. Respondiendo OK.");
            enviar_respuesta(cliente_socket, OK);
            break;

        case PEDIDO_READ_MEM:
            log_info(logger, "Peticiones de READ MEM no implementadas. Respondiendo OK.");
            enviar_respuesta(cliente_socket, OK);
            break;

        case PEDIDO_WRITE_MEM:
            log_info(logger, "Peticiones de WRITE MEM no implementadas. Respondiendo OK.");
            enviar_respuesta(cliente_socket, OK);
            break;

        case PEDIDO_SUB:
            log_info(logger, "Peticiones de SUB no implementadas. Respondiendo OK.");
            enviar_respuesta(cliente_socket, OK);
            break;

        case PEDIDO_SUM:
            log_info(logger, "Peticiones de escritura de SUM no implementadas. Respondiendo OK.");
            enviar_respuesta(cliente_socket, OK);
            break;

        case PEDIDO_JNZ:
            log_info(logger, "Peticiones de lectura de JNZ no implementadas. Respondiendo OK.");
            enviar_respuesta(cliente_socket, OK);
            break;

        case PEDIDO_LOG:
            log_info(logger, "Peticiones de LOG no implementadas. Respondiendo OK.");
            enviar_respuesta(cliente_socket, OK);
            break;

        case ERROROPCODE:
            log_error(logger, "Cliente desconectado de %s... con cop -1", server_name);
            break;

        default:
            log_error(logger, "Algo anduvo mal en el servidor de %s, Cod OP: %d", server_name, cop);
            break;
        }
    }

    log_warning(logger, "El cliente se desconectó de %s server", server_name);
    return;
}

int server_escuchar(t_log *logger, char *server_name, int server_socket)
{
    int cliente_socket = esperar_cliente(server_socket, logger);

    if (cliente_socket != -1)
    {
        pthread_t hilo;
        t_procesar_conexion_args *args = malloc(sizeof(t_procesar_conexion_args));
        args->log = logger;
        args->fd = cliente_socket;
        args->server_name = server_name;
        pthread_create(&hilo, NULL, (void *)procesar_conexion_memoria, (void *)args);
        pthread_detach(hilo);
        return 1;
    }
    return 0;
}