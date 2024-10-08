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
        if (recv(cliente_socket, &cop, sizeof(op_cod), 0) != sizeof(op_cod))
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
            uint32_t pid, base, limite;
            recibir_inicializar_proceso(&pid, &base, &limite, cliente_socket);
            crear_proceso(pid, base, limite); // Crear proceso y su contexto
            log_info(logger, "Proceso inicializado con PID: %d", pid);
            break;

        case FINALIZAR_PROCESO:
            uint32_t pid_a_finalizar;
            recibir_finalizar_proceso(&pid_a_finalizar, cliente_socket);
            liberar_proceso(pid_a_finalizar);
            log_info(logger, "Proceso con PID: %d finalizado", pid_a_finalizar);
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
        case PEDIDO_RESIZE:
            log_info(logger, "Peticiones de resize no implementadas. Respondiendo OK.");
            enviar_respuesta(cliente_socket, OK);
            break;

        case PEDIDO_MOV_IN:
            log_info(logger, "Peticiones de MOV IN no implementadas. Respondiendo OK.");
            enviar_respuesta(cliente_socket, OK);
            break;

        case PEDIDO_MOV_OUT:
            log_info(logger, "Peticiones de MOV OUT no implementadas. Respondiendo OK.");
            enviar_respuesta(cliente_socket, OK);
            break;

        case PEDIDO_COPY_STRING:
            log_info(logger, "Peticiones de COPY STRING no implementadas. Respondiendo OK.");
            enviar_respuesta(cliente_socket, OK);
            break;

        case PEDIDO_ESCRIBIR_DATO_STDIN:
            log_info(logger, "Peticiones de escritura de STDIN no implementadas. Respondiendo OK.");
            enviar_respuesta(cliente_socket, OK);
            break;

        case PEDIDO_A_LEER_DATO_STDOUT:
            log_info(logger, "Peticiones de lectura de STDOUT no implementadas. Respondiendo OK.");
            enviar_respuesta(cliente_socket, OK);
            break;

        case PEDIDO_MARCO:
            log_info(logger, "Peticiones de marco no implementadas. Respondiendo OK.");
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

void enviar_respuesta(int cliente_socket, op_cod response)
{
    send(cliente_socket, &response, sizeof(op_cod), 0);
}
