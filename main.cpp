/**
 * @file main.cpp
 * @brief Decodificador de Protocolo Industrial PRT-7
 * @author [Tu Nombre]
 * @date 2025
 * @version 1.0
 * 
 * @details
 * Este programa implementa un decodificador para el protocolo PRT-7, que recibe
 * tramas de tipo LOAD y MAP desde un puerto serial o archivo de simulación.
 * Utiliza una lista doblemente enlazada para almacenar fragmentos decodificados
 * y una lista circular como rotor de mapeo (cifrado César dinámico).
 * 
 * @section compile Compilación
 * @code
 * mkdir build
 * cd build
 * cmake ..
 * make
 * @endcode
 * 
 * @section usage Uso
 * @code
 * ./prtdcd --sim entrada.txt
 * ./prtdcd --serial /dev/ttyUSB0
 * @endcode
 */

#include <iostream>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cctype>
#include <cerrno>

#ifdef __linux__
#include <termios.h>
#include <unistd.h>
#include <fcntl.h>
#endif

using std::cout;
using std::endl;

/**************************************************************************
 * Declaraciones adelantadas
 **************************************************************************/
class ListaDeCarga;
class RotorDeMapeo;

/**************************************************************************
 * @class TramaBase
 * @brief Clase base abstracta para todas las tramas del protocolo PRT-7
 * 
 * @details
 * Define la interfaz común para las tramas LOAD y MAP. Utiliza polimorfismo
 * para permitir el procesamiento uniforme de diferentes tipos de tramas.
 * El destructor virtual es crítico para la correcta liberación de memoria.
 **************************************************************************/
class TramaBase {
public:
    /**
     * @brief Procesa la trama y modifica las estructuras de datos
     * @param carga Puntero a la lista de carga donde se almacenan fragmentos decodificados
     * @param rotor Puntero al rotor de mapeo que realiza la decodificación
     * @pre carga y rotor deben estar inicializados
     * @post Las estructuras pueden ser modificadas según el tipo de trama
     */
    virtual void procesar(ListaDeCarga* carga, RotorDeMapeo* rotor) = 0;
    
    /**
     * @brief Destructor virtual para permitir polimorfismo correcto
     * @details Esencial para evitar fugas de memoria al eliminar objetos derivados
     */
    virtual ~TramaBase() {}
};

/**************************************************************************
 * Estructuras de Nodos
 **************************************************************************/

/**
 * @struct NodoCarga
 * @brief Nodo para la lista doblemente enlazada de fragmentos decodificados
 */
struct NodoCarga {
    char dato;           ///< Carácter decodificado almacenado
    NodoCarga* prev;     ///< Puntero al nodo anterior
    NodoCarga* next;     ///< Puntero al nodo siguiente
    
    /**
     * @brief Constructor del nodo
     * @param d Carácter a almacenar
     */
    NodoCarga(char d) : dato(d), prev(nullptr), next(nullptr) {}
};

/**
 * @struct NodoRotor
 * @brief Nodo para la lista circular del rotor de mapeo
 */
struct NodoRotor {
    char c;              ///< Carácter del alfabeto (A-Z)
    NodoRotor* prev;     ///< Puntero al nodo anterior (circular)
    NodoRotor* next;     ///< Puntero al nodo siguiente (circular)
    
    /**
     * @brief Constructor del nodo
     * @param ch Carácter del alfabeto
     */
    NodoRotor(char ch) : c(ch), prev(nullptr), next(nullptr) {}
};

/**************************************************************************
 * @class ListaDeCarga
 * @brief Lista doblemente enlazada para almacenar fragmentos decodificados
 * 
 * @details
 * Implementación manual (sin STL) de una lista doblemente enlazada que
 * mantiene el orden de los fragmentos de mensaje a medida que se decodifican.
 **************************************************************************/
class ListaDeCarga {
private:
    NodoCarga* head;     ///< Puntero al primer nodo
    NodoCarga* tail;     ///< Puntero al último nodo

public:
    /**
     * @brief Constructor por defecto
     * @post Lista vacía inicializada
     */
    ListaDeCarga() : head(nullptr), tail(nullptr) {}
    
    /**
     * @brief Destructor - libera toda la memoria de los nodos
     * @details Recorre la lista y elimina cada nodo individualmente
     */
    ~ListaDeCarga() {
        NodoCarga* cur = head;
        while (cur) {
            NodoCarga* nx = cur->next;
            delete cur;
            cur = nx;
        }
    }

    /**
     * @brief Inserta un carácter al final de la lista
     * @param dato Carácter a insertar
     * @post El carácter se agrega al final, manteniendo el orden de llegada
     */
    void insertarAlFinal(char dato) {
        NodoCarga* n = new NodoCarga(dato);
        if (!tail) {
            head = tail = n;
        } else {
            tail->next = n;
            n->prev = tail;
            tail = n;
        }
    }

    /**
     * @brief Imprime el mensaje actual entre corchetes
     * @details Formato: [H][O][L][A]
     */
    void imprimirMensaje() {
        cout << "Mensaje: ";
        NodoCarga* cur = head;
        while (cur) {
            cout << "[" << (cur->dato == ' ' ? ' ' : cur->dato) << "]";
            cur = cur->next;
        }
        cout << endl;
    }

    /**
     * @brief Imprime el mensaje final completo sin corchetes
     * @details Se llama al finalizar el procesamiento de todas las tramas
     */
    void imprimirMensajeFinal() {
        cout << "MENSAJE OCULTO ENSAMBLADO:" << endl;
        NodoCarga* cur = head;
        while (cur) {
            cout << cur->dato;
            cur = cur->next;
        }
        cout << endl;
    }
};

/**************************************************************************
 * @class RotorDeMapeo
 * @brief Lista circular doblemente enlazada que implementa un rotor de cifrado
 * 
 * @details
 * Simula un disco de cifrado similar a una rueda de César. Contiene el
 * alfabeto A-Z en una lista circular que puede rotarse para cambiar el mapeo.
 * La posición actual del rotor determina cómo se decodifica cada carácter.
 **************************************************************************/
class RotorDeMapeo {
private:
    NodoRotor* head;     ///< Puntero a la posición 'cero' actual del rotor
    int size;            ///< Tamaño del rotor (26 letras)

public:
    /**
     * @brief Constructor - inicializa el rotor con A-Z
     * @post Rotor circular creado con 26 nodos, head apuntando a 'A'
     */
    RotorDeMapeo() : head(nullptr), size(0) {
        // Construir lista circular A..Z
        NodoRotor* first = nullptr;
        NodoRotor* prev = nullptr;
        for (char ch = 'A'; ch <= 'Z'; ++ch) {
            NodoRotor* n = new NodoRotor(ch);
            if (!first) first = n;
            if (prev) {
                prev->next = n;
                n->prev = prev;
            }
            prev = n;
            ++size;
        }
        // Cerrar la circularidad
        if (first && prev) {
            first->prev = prev;
            prev->next = first;
            head = first;
        }
    }

    /**
     * @brief Destructor - libera memoria del rotor
     * @details Rompe la circularidad antes de eliminar para evitar loops infinitos
     */
    ~RotorDeMapeo() {
        if (!head) return;
        // Romper circularidad
        head->prev->next = nullptr;
        NodoRotor* cur = head;
        while (cur) {
            NodoRotor* nx = cur->next;
            delete cur;
            cur = nx;
        }
    }

    /**
     * @brief Rota el rotor N posiciones
     * @param N Número de posiciones a rotar (+ derecha, - izquierda)
     * @post head se mueve N posiciones en la lista circular
     * @details Maneja correctamente rotaciones positivas y negativas usando módulo
     */
    void rotar(int N) {
        if (!head || size <= 1) return;
        
        // Calcular desplazamiento efectivo
        int effective = N % size;
        if (effective < 0) effective += size;
        
        // Mover head
        for (int i = 0; i < effective; ++i) {
            head = head->next;
        }
        
        cout << " -> ROTANDO ROTOR " << (N >= 0 ? "+" : "") << N 
             << " (efectivo: +" << effective << ")" << endl;
    }

    /**
     * @brief Obtiene el carácter mapeado según la rotación actual del rotor
     * @param in Carácter de entrada a decodificar
     * @return Carácter decodificado según el mapeo actual
     * @details 
     * - Los espacios se devuelven sin cambios
     * - Las minúsculas se convierten a mayúsculas
     * - El mapeo se realiza encontrando la posición relativa desde head
     */
    char getMapeo(char in) {
        if (in == ' ') return ' ';
        
        // Normalizar a mayúscula
        if (in >= 'a' && in <= 'z') 
            in = char(in - 'a' + 'A');
        
        if (in < 'A' || in > 'Z') 
            return in;
        
        // Encontrar posición y devolver carácter mapeado
        int index = in - 'A';
        NodoRotor* cur = head;
        for (int i = 0; i < index; ++i) {
            cur = cur->next;
        }
        return cur->c;
    }

    /**
     * @brief Imprime el estado actual del rotor para debug
     * @details Muestra las 26 letras desde la posición head
     */
    void imprimirEstado() {
        if (!head) return;
        cout << "Estado rotor (desde head): ";
        NodoRotor* cur = head;
        for (int i = 0; i < size; ++i) {
            cout << cur->c;
            cur = cur->next;
        }
        cout << endl;
    }
};

/**************************************************************************
 * @class TramaLoad
 * @brief Trama de tipo LOAD que contiene un fragmento de dato
 * 
 * @details
 * Representa una trama "L,X" donde X es un carácter que debe ser decodificado
 * usando el estado actual del rotor y agregado a la lista de carga.
 **************************************************************************/
class TramaLoad : public TramaBase {
private:
    char fragmento;      ///< Carácter a decodificar

public:
    /**
     * @brief Constructor
     * @param f Carácter fragmento de la trama
     */
    TramaLoad(char f) : fragmento(f) {}
    
    /**
     * @brief Procesa la trama LOAD
     * @param carga Lista donde se insertará el fragmento decodificado
     * @param rotor Rotor usado para decodificar el fragmento
     * @post El fragmento decodificado se agrega al final de la lista de carga
     */
    virtual void procesar(ListaDeCarga* carga, RotorDeMapeo* rotor) {
        cout << "Trama: [L, " 
             << (fragmento == ' ' ? "Space" : std::string(1, fragmento)) 
             << "] -> Procesando...";
        
        char dec = rotor->getMapeo(fragmento);
        
        cout << " -> Fragmento '" << fragmento 
             << "' decodificado como '" << dec << "'. ";
        
        carga->insertarAlFinal(dec);
        carga->imprimirMensaje();
    }
    
    /**
     * @brief Destructor
     */
    virtual ~TramaLoad() {}
};

/**************************************************************************
 * @class TramaMap
 * @brief Trama de tipo MAP que modifica la rotación del rotor
 * 
 * @details
 * Representa una trama "M,N" donde N es un entero que indica cuántas
 * posiciones debe rotar el rotor (positivo o negativo).
 **************************************************************************/
class TramaMap : public TramaBase {
private:
    int desplazamiento;  ///< Número de posiciones a rotar

public:
    /**
     * @brief Constructor
     * @param d Desplazamiento a aplicar al rotor
     */
    TramaMap(int d) : desplazamiento(d) {}
    
    /**
     * @brief Procesa la trama MAP
     * @param carga No se utiliza en tramas MAP
     * @param rotor Rotor que será rotado
     * @post El rotor se rota N posiciones
     */
    virtual void procesar(ListaDeCarga* carga, RotorDeMapeo* rotor) {
        cout << "Trama: [M," << desplazamiento << "] -> Procesando... ";
        rotor->rotar(desplazamiento);
        rotor->imprimirEstado();
    }
    
    /**
     * @brief Destructor
     */
    virtual ~TramaMap() {}
};

/**************************************************************************
 * @class SerialReader
 * @brief Clase para leer datos desde puerto serial o archivo de simulación
 * 
 * @details
 * Intenta abrir un puerto serial en Linux. Si falla, intenta abrir como
 * archivo de texto para simulación. Soporta lectura línea por línea.
 **************************************************************************/
class SerialReader {
private:
    FILE* f;             ///< File pointer para lectura
    bool is_serial;      ///< Indica si es puerto serial real
#ifdef __linux__
    int fd;              ///< File descriptor (Linux)
#endif

public:
    /**
     * @brief Constructor por defecto
     */
    SerialReader() : f(nullptr), is_serial(false)
#ifdef __linux__
    , fd(-1)
#endif
    {}

    /**
     * @brief Destructor - cierra puerto/archivo
     */
    ~SerialReader() {
#ifdef __linux__
        if (fd != -1) {
            close(fd);
        }
#endif
        if (f) fclose(f);
    }

    /**
     * @brief Abre un puerto serial o archivo de simulación
     * @param path Ruta del dispositivo (/dev/ttyUSB0) o archivo
     * @param baud Baud rate para puerto serial (default: 9600)
     * @return true si se abrió correctamente
     * @details Intenta primero como puerto serial (Linux), luego como archivo
     */
    bool abrir(const char* path, int baud = 9600) {
#ifdef __linux__
        // Intentar como puerto serial
        fd = open(path, O_RDONLY | O_NOCTTY | O_NONBLOCK);
        if (fd != -1) {
            struct termios tty;
            if (tcgetattr(fd, &tty) == 0) {
                cfmakeraw(&tty);
                cfsetspeed(&tty, B9600);
                tty.c_cflag &= ~PARENB;
                tty.c_cflag &= ~CSTOPB;
                tty.c_cflag &= ~CSIZE;
                tty.c_cflag |= CS8;
                tty.c_cflag |= CREAD;
                tcsetattr(fd, TCSANOW, &tty);
                
                f = fdopen(fd, "r");
                if (!f) {
                    close(fd); 
                    fd = -1;
                } else {
                    is_serial = true;
                    cout << "Conexión serial abierta en " << path << endl;
                    return true;
                }
            } else {
                close(fd); 
                fd = -1;
            }
        }
#endif
        // Fallback: archivo de simulación
        f = fopen(path, "r");
        if (!f) {
            cout << "Error abriendo '" << path << "': " 
                 << strerror(errno) << endl;
            return false;
        }
        is_serial = false;
        cout << "Abierto archivo de simulación: " << path << endl;
        return true;
    }

    /**
     * @brief Lee una línea del puerto/archivo
     * @param outBuf Buffer de salida
     * @param maxLen Tamaño máximo del buffer
     * @return true si se leyó algo
     * @post outBuf contiene la línea sin \\r\\n
     */
    bool leerLinea(char* outBuf, size_t maxLen) {
        if (!f) return false;
        if (fgets(outBuf, (int)maxLen, f) == nullptr) {
            return false;
        }
        // Eliminar \r\n
        size_t L = strlen(outBuf);
        while (L > 0 && (outBuf[L-1] == '\n' || outBuf[L-1] == '\r')) {
            outBuf[--L] = '\0';
        }
        return true;
    }
};

/**************************************************************************
 * Funciones auxiliares
 **************************************************************************/

/**
 * @brief Elimina espacios en blanco al inicio y final de una cadena
 * @param s Cadena a modificar (in-place)
 * @post s contiene la cadena sin espacios al inicio/final
 */
void trim(char* s) {
    // Trim izquierdo
    int i = 0;
    while (s[i] && isspace((unsigned char)s[i])) ++i;
    if (i > 0) memmove(s, s + i, strlen(s + i) + 1);
    
    // Trim derecho
    int L = (int)strlen(s);
    while (L > 0 && isspace((unsigned char)s[L-1])) 
        s[--L] = '\0';
}

/**
 * @brief Parsea una línea de texto y crea la trama correspondiente
 * @param lineaC Línea a parsear (formato: "L,X" o "M,N")
 * @return Puntero a TramaBase* (debe liberarse con delete) o nullptr si inválida
 * @details
 * Formatos válidos:
 * - L,X : TramaLoad con carácter X
 * - L,Space : TramaLoad con espacio
 * - M,N : TramaMap con desplazamiento N (puede ser negativo)
 */
TramaBase* parseLinea(const char* lineaC) {
    char buffer[128];
    strncpy(buffer, lineaC, sizeof(buffer));
    buffer[sizeof(buffer)-1] = '\0';
    trim(buffer);
    
    if (strlen(buffer) == 0) return nullptr;

    // Tokenizar por coma
    char* token = strtok(buffer, ",");
    if (!token) return nullptr;
    trim(token);
    if (strlen(token) == 0) return nullptr;

    // Trama LOAD
    if ((token[0] == 'L' || token[0] == 'l') && (token[1] == '\0')) {
        char* arg = strtok(nullptr, ",");
        if (!arg) {
            cout << "Trama L sin argumento." << endl;
            return nullptr;
        }
        trim(arg);
        
        // Detectar "Space"
        if ((arg[0] == 'S' || arg[0] == 's') && (strcasecmp(arg, "Space") == 0)) {
            return new TramaLoad(' ');
        }
        
        // Carácter único
        if (strlen(arg) == 1) {
            return new TramaLoad(arg[0]);
        }
        
        // Tomar primer carácter
        if (strlen(arg) > 1) {
            return new TramaLoad(arg[0]);
        }
        return nullptr;
    } 
    // Trama MAP
    else if ((token[0] == 'M' || token[0] == 'm') && (token[1] == '\0')) {
        char* arg = strtok(nullptr, ",");
        if (!arg) {
            cout << "Trama M sin argumento." << endl;
            return nullptr;
        }
        trim(arg);
        int n = atoi(arg);
        return new TramaMap(n);
    } 
    else {
        cout << "Tipo de trama desconocido: " << token << endl;
        return nullptr;
    }
}

/**************************************************************************
 * @brief Función principal del decodificador PRT-7
 * 
 * @param argc Número de argumentos
 * @param argv Array de argumentos
 * @return 0 si éxito, 1 si error
 * 
 * @details
 * Inicializa las estructuras de datos, abre la conexión serial/archivo,
 * lee y procesa tramas en un bucle, y finalmente muestra el mensaje decodificado.
 * 
 * @section args Argumentos
 * - --sim <archivo> : Modo simulación con archivo de texto
 * - --serial <dispositivo> : Modo serial real (Linux)
 * 
 * @section ejemplo Ejemplo de uso
 * @code
 * ./prtdcd --sim entrada.txt
 * ./prtdcd --serial /dev/ttyUSB0
 * @endcode
 **************************************************************************/
int main(int argc, char** argv) {
    cout << "Iniciando Decodificador PRT-7. Preparando estructuras..." << endl;
    
    ListaDeCarga miCarga;
    RotorDeMapeo miRotor;

    // Validar argumentos
    if (argc < 2) {
        cout << "Uso: " << argv[0] 
             << " --sim <archivo_simulacion>   (o)  --serial <dispositivo>" << endl;
        cout << "Ejemplo de archivo_simulacion (lineas): "
             << "L,H  L,O  L,L  M,2  L,A  L,Space  L,W  M,-2  L,O  L,R  L,L  L,D" << endl;
        cout << "Saliendo (ningún archivo ni serial especificado)." << endl;
        return 1;
    }

    const char* modo = argv[1];
    const char* ruta = (argc >= 3 ? argv[2] : nullptr);
    
    SerialReader reader;
    if (!ruta) {
        cout << "Falta ruta (archivo o dispositivo)." << endl;
        return 1;
    }

    // Abrir conexión
    bool opened = reader.abrir(ruta);
    if (!opened) {
        cout << "No se pudo abrir ruta: " << ruta << endl;
        return 1;
    }

    cout << "Conexión establecida. Esperando tramas..." << endl << endl;

    // Bucle principal de procesamiento
    char linea[256];
    while (reader.leerLinea(linea, sizeof(linea))) {
        cout << "Trama recibida: [" << linea << "] ";
        
        // Parsear trama
        TramaBase* trama = parseLinea(linea);
        if (!trama) {
            cout << " -> Trama inválida. Se ignora." << endl;
            continue;
        }
        
        // Procesar trama (polimorfismo)
        trama->procesar(&miCarga, &miRotor);
        
        // Liberar memoria
        delete trama;
        cout << endl;
    }

    // Mostrar resultado final
    cout << "\n---\nFlujo de datos terminado.\n";
    miCarga.imprimirMensajeFinal();
    cout << "---\nLiberando memoria... Sistema apagado." << endl;

    return 0;
}