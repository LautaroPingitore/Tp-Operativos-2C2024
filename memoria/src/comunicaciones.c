#include "include/comunicaciones.h"


static void procesar_conexion_memoria(void *void_args)
{
    t_procesar_conexion_args *args = (t_procesar_conexion_args *)void_args;
    t_log *logger = args->log;
    int cliente_socket = args->fd;
    char *server_name = args->server_name;
    free(args);

    op_code cod_op;
    while (cliente_socket != -1)
    {

        if (recv(cliente_socket, &cod_op, sizeof(op_code), 0) != sizeof(op_code))
        {
            log_debug(logger, "Cliente desconectado.\n");
            return;
        }

        switch (cod_op)
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
        case PEDIDO_SET:{
            uint32_t pid, registro, valor;
            recibir_set(&pid, &registro, &valor, cliente_socket);
            t_contexto_ejecucion *contexto = obtener_contexto(pid);
            switch (registro) {
                case REG_AX: contexto->AX = valor; break;
                case REG_BX: contexto->BX = valor; break;
                case REG_CX: contexto->CX = valor; break;
                case REG_DX: contexto->DX = valor; break;
                case REG_EX: contexto->EX = valor; break;
                case REG_FX: contexto->FX = valor; break;
                case REG_GX: contexto->GX = valor; break;
                case REG_HX: contexto->HX = valor; break;
                default: enviar_respuesta(cliente_socket, "ERROR: Registro no válido"); continue;
                }
            enviar_respuesta(cliente_socket, OK);
            log_info(logger, "Seteado registro %d a valor %d para PID %d", registro, valor, pid);
            break;
        }
        case PEDIDO_READ_MEM: {
                uint32_t pid, direccion_logica;
                recibir_read_mem(&pid, &direccion_logica, cliente_socket);
                t_contexto_ejecucion *contexto = obtener_contexto(pid);
                uint32_t valor = *(uint32_t *)((char *)memoriaUsuario + direccion_logica);
                enviar_respuesta(cliente_socket, (char *)&valor); // Enviamos el valor leído
                log_info(logger, "Se leyó memoria en dirección %d para PID %d", direccion_logica, pid);
            break;
        }
        case PEDIDO_WRITE_MEM: {
                uint32_t pid, direccion_logica, valor;
                recibir_write_mem(&pid, &direccion_logica, &valor, cliente_socket);
                *(uint32_t *)((char *)memoriaUsuario + direccion_logica) = valor;
                enviar_respuesta(cliente_socket, OK);
                log_info(logger, "Se escribió valor %d en dirección %d para PID %d", valor, direccion_logica, pid);
                break;
        }

        case PEDIDO_SUB: {
                uint32_t pid, registro1, registro2;
                recibir_sub(&pid, &registro1, &registro2, cliente_socket);
                t_contexto_ejecucion *contexto = obtener_contexto(pid);
                uint32_t valor1 = 0, valor2 = 0;

                // Obtener valores de los registros
                switch (registro1) {
                    case REG_AX: valor1 = contexto->AX; break;
                    case REG_BX: valor1 = contexto->BX; break;
                    case REG_CX: valor1 = contexto->CX; break;
                    case REG_DX: valor1 = contexto->DX; break;
                    case REG_EX: valor1 = contexto->EX; break;
                    case REG_FX: valor1 = contexto->FX; break;
                    case REG_GX: valor1 = contexto->GX; break;
                    case REG_HX: valor1 = contexto->HX; break;
                    default: enviar_respuesta(cliente_socket, "ERROR: Registro no válido"); continue;
                }

                switch (registro2) {
                    case REG_AX: valor2 = contexto->AX; break;
                    case REG_BX: valor2 = contexto->BX; break;
                    case REG_CX: valor2 = contexto->CX; break;
                    case REG_DX: valor2 = contexto->DX; break;
                    case REG_EX: valor2 = contexto->EX; break;
                    case REG_FX: valor2 = contexto->FX; break;
                    case REG_GX: valor2 = contexto->GX; break;
                    case REG_HX: valor2 = contexto->HX; break;
                    default: enviar_respuesta(cliente_socket, "ERROR: Registro no válido"); continue;
                }

                uint32_t resultado = valor1 - valor2; // Realizar la resta
                switch (registro1) {
                    case REG_AX: contexto->AX = resultado; break;
                    case REG_BX: contexto->BX = resultado; break;
                    case REG_CX: contexto->CX = resultado; break;
                    case REG_DX: contexto->DX = resultado; break;
                    case REG_EX: contexto->EX = resultado; break;
                    case REG_FX: contexto->FX = resultado; break;
                    case REG_GX: contexto->GX = resultado; break;
                    case REG_HX: contexto->HX = resultado; break;
                    default: enviar_respuesta(cliente_socket, "ERROR: Registro no válido"); continue;
                }
                enviar_respuesta(cliente_socket, OK);
                log_info(logger, "Se restó el valor de registro %d de %d para PID %d", registro2, registro1, pid);
                break;
            }

        case PEDIDO_SUM: {
                uint32_t pid, registro1, registro2;
                recibir_sum(&pid, &registro1, &registro2, cliente_socket);
                t_contexto_ejecucion *contexto = obtener_contexto(pid);
                uint32_t valor1 = 0, valor2 = 0;

                // Obtener valores de los registros
                switch (registro1) {
                    case REG_AX: valor1 = contexto->AX; break;
                    case REG_BX: valor1 = contexto->BX; break;
                    case REG_CX: valor1 = contexto->CX; break;
                    case REG_DX: valor1 = contexto->DX; break;
                    case REG_EX: valor1 = contexto->EX; break;
                    case REG_FX: valor1 = contexto->FX; break;
                    case REG_GX: valor1 = contexto->GX; break;
                    case REG_HX: valor1 = contexto->HX; break;
                    default: enviar_respuesta(cliente_socket, "ERROR: Registro no válido"); continue;
                }

                switch (registro2) {
                    case REG_AX: valor2 = contexto->AX; break;
                    case REG_BX: valor2 = contexto->BX; break;
                    case REG_CX: valor2 = contexto->CX; break;
                    case REG_DX: valor2 = contexto->DX; break;
                    case REG_EX: valor2 = contexto->EX; break;
                    case REG_FX: valor2 = contexto->FX; break;
                    case REG_GX: valor2 = contexto->GX; break;
                    case REG_HX: valor2 = contexto->HX; break;
                    default: enviar_respuesta(cliente_socket, "ERROR: Registro no válido"); continue;
                }

                uint32_t resultado = valor1 + valor2; // Realizar la suma
                switch (registro1) {
                    case REG_AX: contexto->AX = resultado; break;
                    case REG_BX: contexto->BX = resultado; break;
                    case REG_CX: contexto->CX = resultado; break;
                    case REG_DX: contexto->DX = resultado; break;
                    case REG_EX: contexto->EX = resultado; break;
                    case REG_FX: contexto->FX = resultado; break;
                    case REG_GX: contexto->GX = resultado; break;
                    case REG_HX: contexto->HX = resultado; break;
                    default: enviar_respuesta(cliente_socket, "ERROR: Registro no válido"); continue;
                }
                enviar_respuesta(cliente_socket, OK);
                log_info(logger, "Se sumó el valor de registro %d y %d para PID %d", registro1, registro2, pid);
                break;
            }

        case PEDIDO_JNZ: {
                uint32_t pid, pc_actual, valor_condicion;
                recibir_jnz(&pid, &pc_actual, &valor_condicion, cliente_socket);
                if (valor_condicion != 0) {
                    // Cambiar el PC si la condición se cumple
                    cambiar_pc(pid, pc_actual);
                    enviar_respuesta(cliente_socket, OK);
                    log_info(logger, "JNZ: Se cambió el PC para PID %d", pid);
                } else {
                    enviar_respuesta(cliente_socket, OK);
                    log_info(logger, "JNZ: La condición no se cumplió para PID %d", pid);
                }
                break;
            }

        case PEDIDO_LOG: {
                char mensaje[256];
                recibir_log(mensaje, cliente_socket);
                log_info(logger, "Log recibido: %s", mensaje);
                enviar_respuesta(cliente_socket, OK);
                break;
            }

        case ERROROPCODE:
            log_error(logger, "Cliente desconectado de %s... con cod_op -1", server_name);
            break;

        default:
            log_error(logger, "Algo anduvo mal en el servidor de %s, Cod OP: %d", server_name, cod_op);
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

// Enviar respuesta a la CPU
void enviar_respuesta(int socket_cpu, const char *mensaje) {
    // Enviar el mensaje a través del socket de la CPU
    send(socket_cpu, mensaje, strlen(mensaje) + 1, 0); // +1 para incluir el terminador nulo
}