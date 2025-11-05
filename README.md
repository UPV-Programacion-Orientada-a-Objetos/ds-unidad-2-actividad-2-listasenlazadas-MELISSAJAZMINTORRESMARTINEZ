[![Review Assignment Due Date](https://classroom.github.com/assets/deadline-readme-button-22041afd0340ce965d47ae6ef1cefeee28c7c493a6346c4f15d667ab976d596c.svg)](https://classroom.github.com/a/zp6HS-Kj)
[![Open in Codespaces](https://classroom.github.com/assets/launch-codespace-2972f46106e565e64193e422d61a12cf1da4916b45550586e14ef0a7c637dd04.svg)](https://classroom.github.com/open-in-codespaces?assignment_repo_id=21478928)
# Caso de Estudio: Decodificador de Protocolo Industrial (PRT-7)

## Definición del Problema a Resolver (El Caso de Estudio)

Ustedes han sido contratados como ingenieros de software senior en una firma de ciberseguridad industrial. Un competidor ha estado interceptando la telemetría de sus sensores (simulados por un dispositivo Arduino), pero no han podido descifrarla.

Se ha descubierto que el Arduino no envía datos encriptados, sino que transmite un **protocolo de ensamblaje de mensajes ocultos** llamado PRT-7. El protocolo no envía un mensaje, sino las *instrucciones* para construir el mensaje.

El Arduino envía dos tipos de tramas por el puerto serial:

1. **Tramas de Carga (LOAD):** Contienen un fragmento de datos (un carácter).
2. **Tramas de Mapeo (MAP):** Contienen instrucciones para reordenar los fragmentos de datos ya recibidos.

Su misión es construir un software en C++ que lea este flujo de tramas desde el puerto serial y utilice las estructuras de datos correctas para ensamblar el mensaje oculto.

  * Las tramas **LOAD** deben almacenarse en una **Lista Doblemente Enlazada**, ya que el orden de llegada es crucial.
  * Las tramas **MAP** operan sobre una **Lista Circular**, que actúa como un "disco de cifrado" (similar a una Rueda de César) que mapea un carácter a otro.

El desafío es que las tramas MAP modifican el estado de la Lista Circular *mientras* se procesan las tramas LOAD, haciendo que el significado de cada fragmento de dato cambie dinámicamente.

---

## Temas Relacionados y Necesarios

Para resolver este caso de estudio, los alumnos deberán dominar la integración de los siguientes conceptos:

| Tema Principal | Concepto a Aplicar |
| :--- | :--- |
| **POO Avanzada (Herencia/Polimorfismo)** | **Clase Base Abstracta** (`TramaBase`) y **Clases Derivadas** (`TramaLoad`, `TramaMap`). |
| **Listas Doblemente Enlazadas (Manual)** | Implementación de una `ListaDeCarga` para almacenar las tramas recibidas en orden (`TramaBase*`). |
| **Listas Circulares (Manual)** | Implementación de un `RotorDeMapeo` (una Lista Circular Doblemente Enlazada) que contiene el alfabeto (A-Z) y puede rotar. |
| **Punteros y Gestión de Memoria** | Uso de `new`/`delete` para crear nodos y objetos Trama. Gestión de punteros `previo` y `siguiente`. |
| **Comunicación Serial (Hardware)** | Lectura de datos en tiempo real desde un puerto COM (simulado por el Arduino). |
| **Análisis de Protocolos** | Interpretación de cadenas de texto (`L,A` o `M,5`) para instanciar los objetos correctos. |

---

## Definición y Detalles del Proceso a Desarrollar

### A. Diseño de la Jerarquía de Clases (POO)

El sistema debe basarse en una interfaz común para todas las tramas recibidas.

1. **Clase Base Abstracta (`TramaBase`):**

      * **Propósito:** Define la interfaz para cualquier trama que llegue del puerto serial.
      * **Método Virtual Puro:** `virtual void procesar(ListaDeCarga* carga, RotorDeMapeo* rotor) = 0;`
      * **Destructor:** `virtual ~TramaBase() {}` (Destructor virtual OBLIGATORIO para la limpieza de memoria polimórfica).

2. **Clases Concretas (Derivadas):**

      * `TramaLoad : public TramaBase`: Representa una trama `L,X` (ej. `L,A`). Almacena el carácter `X`.
      * `TramaMap : public TramaBase`: Representa una trama `M,N` (ej. `M,5` o `M,-3`). Almacena el entero `N`.

### B. Estructuras de Datos "A Mano"

1. **`RotorDeMapeo` (Lista Circular Doblemente Enlazada):**

      * Contiene nodos con `char` (A-Z).
      * Debe tener un puntero `cabeza` que indica la posición 'cero' actual.
      * **Método:** `rotar(int N)`: Mueve el puntero `cabeza` N posiciones (positivo o negativo) de forma circular.
      * **Método:** `getMapeo(char in)`: Busca el carácter `in` en la lista, encuentra su posición relativa a `cabeza`, y devuelve el carácter que está en la posición 'cero' (la `cabeza`) si `in` estuviera en esa posición (¡lógica compleja de mapeo\!).

2. **`ListaDeCarga` (Lista Doblemente Enlazada):**

      * Contiene nodos que almacenan `char` (los datos decodificados).
      * Debe implementar `insertarAlFinal(char dato)`.
      * Debe implementar `imprimirMensaje()`.

### C. Proceso de Solución Detallado (La Lógica del Decodificador)

El programa debe gestionar dos estructuras simultáneamente:

1. **Inicialización:** Crear la `ListaDeCarga` (vacía) y el `RotorDeMapeo` (cargado con A-Z, `cabeza` apuntando a 'A').
2.  **Lectura Serial:** El programa debe abrir el puerto COM donde está el Arduino. Leerá líneas de texto (ej. "L,G", "M,-2").
3. **Bucle de Procesamiento (El Reto):**
      * Para cada línea leída:
          * **Parsear:** Determinar si es 'L' (Load) o 'M' (Map).
          * **Instanciar:**
              * Si es "L,G" -\> `TramaBase* trama = new TramaLoad('G');`
              * Si es "M,-2" -\> `TramaBase* trama = new TramaMap(-2);`
          * **Ejecutar:** Llamar a `trama->procesar(&miListaDeCarga, &miRotorDeMapeo);`
          * **Limpiar:** `delete trama;`
4. **Lógica de `procesar()`:**
      * **`TramaMap::procesar(..., RotorDeMapeo* rotor)`:** Simplemente llama a `rotor->rotar(N)`. El disco de cifrado cambia de posición.
      * **`TramaLoad::procesar(ListaDeCarga* carga, RotorDeMapeo* rotor)`:**
          * Toma el carácter de la trama (ej. 'G').
          * Pregunta al rotor: `char decodificado = rotor->getMapeo('G');` (El rotor calcula el carácter mapeado basado en su rotación actual).
          * Inserta el resultado: `carga->insertarAlFinal(decodificado);`
5. **Resultado Final:** Al terminar el flujo de datos, llamar a `miListaDeCarga.imprimirMensaje()` revela el mensaje oculto.

---

## Requerimientos Funcionales y No Funcionales

### Requisitos Funcionales

1. **Comunicación Serial:** El programa debe conectarse y leer con éxito las cadenas de texto enviadas por el Arduino.
2. **POO Polimórfica:** El sistema debe instanciar las clases derivadas correctas (`TramaLoad`, `TramaMap`) y almacenarlas en un puntero `TramaBase*`.
3. **Implementación de Lista Circular:** El `RotorDeMapeo` debe implementarse como una lista circular doblemente enlazada y debe manejar la rotación (positiva y negativa) correctamente.
4. **Implementación de Lista Doble:** La `ListaDeCarga` debe almacenar los caracteres decodificados en el orden correcto.
5. **Decodificación:** El mensaje final debe ensamblarse correctamente basado en la lógica de mapeo y rotación.
6. Uso de CMake: Se deberá generar una sistema redistribuible mediante el uso de CMake.

### Requisitos No Funcionales

1. **Exclusividad de Punteros:** Prohibido el uso de la STL (`std::vector`, `std::list`, `std::string`). El manejo de listas y la lectura/parseo de cadenas debe ser manual (punteros `char*`, `strtok`, etc.).
2. **Gestión de Memoria:** No debe haber fugas de memoria. Cada `new` debe tener su `delete` correspondiente, y el destructor de la clase base debe ser `virtual`.
3. **Robustez:** El programa debe manejar tramas mal formadas del puerto serial (aunque sea de forma básica).

---

## Ejemplo de Entradas y Salidas del Problema en Consola

### Simulación de Interacción (Lo que envía el Arduino)

El Arduino debe estar programado para enviar lo siguiente, con un `delay(1000);` entre cada línea:

```Bash
L,H
L,O
L,L
M,2
L,A
L,Space
L,W
M,-2
L,O
L,R
L,L
L,D
```

### Salida Esperada del Sistema (En la Consola de C++)

```Bash
Iniciando Decodificador PRT-7. Conectando a puerto COM...
Conexión establecida. Esperando tramas...

Trama recibida: [L,H] -> Procesando... -> Fragmento 'H' decodificado como 'H'. Mensaje: [H]
Trama recibida: [L,O] -> Procesando... -> Fragmento 'O' decodificado como 'O'. Mensaje: [H][O]
Trama recibida: [L,L] -> Procesando... -> Fragmento 'L' decodificado como 'L'. Mensaje: [H][O][L]

Trama recibida: [M,2] -> Procesando... -> ROTANDO ROTOR +2. (Ahora 'A' se mapea a 'C')

Trama recibida: [L,A] -> Procesando... -> Fragmento 'A' decodificado como 'C'. Mensaje: [H][O][L][C]
Trama recibida: [L,Space] -> Procesando... -> Fragmento ' ' decodificado como ' '. Mensaje: [H][O][L][C][ ]
Trama recibida: [L,W] -> Procesando... -> Fragmento 'W' decodificado como 'Y'. Mensaje: [H][O][L][C][ ][Y]

Trama recibida: [M,-2] -> Procesando... -> ROTANDO ROTOR -2. (Ahora 'A' se mapea a 'A' de nuevo)

Trama recibida: [L,O] -> Procesando... -> Fragmento 'O' decodificado como 'O'. Mensaje: [H][O][L][C][ ][Y][O]
Trama recibida: [L,R] -> Procesando... -> Fragmento 'R' decodificado como 'R'. Mensaje: [H][O][L][C][ ][Y][O][R]
Trama recibida: [L,L] -> Procesando... -> Fragmento 'L' decodificado como 'L'. Mensaje: [H][O][L][C][ ][Y][O][R][L]
Trama recibida: [L,D] -> Procesando... -> Fragmento 'D' decodificado como 'D'. Mensaje: [H][O][L][C][ ][Y][O][R][L][D]

---
Flujo de datos terminado.
MENSAJE OCULTO ENSAMBLADO:
HOLCY YORLD
---
Liberando memoria... Sistema apagado.
```

-----

## Temas Adicionales de Investigación Necesarios

Para llevar a cabo esta actividad, el alumno deberá investigar y dominar a fondo:

1. **Programación Serial en C++ (El Reto Principal):** Cómo abrir, configurar (Baud Rate, Paridad, etc.) y leer desde un puerto COM (ej. `\\.\COM3` en Windows o `/dev/ttyUSB0` en Linux) usando **solo librerías estándar** (como `<fstream>` o APIs de Win32/POSIX si `<fstream>` no es suficiente).
2. **Destructores Virtuales:** Por qué son **absolutamente necesarios** en la clase `TramaBase` para evitar fugas de memoria masivas al hacer `delete trama;` sobre un puntero de la clase base.
3. **Parseo de Cadenas (C-style):** Técnicas para dividir una cadena leída del serial (ej. "M,-2") en sus componentes (`char*`) usando funciones como `strtok()`, `sscanf()` o `atoi()`, ya que `std::string` y `stringstream` no están permitidas.
4. **Lógica de Listas Circulares Dobles:** La implementación de la rotación eficiente (que no implica mover datos, solo cambiar punteros `head`, `head->previo`, etc.) y la lógica de mapeo relativo.

---

## Entregables

1. Código fuente debidamente documentado mediante Doxygen.
2. Documentación generada mediante Doxygen.
3. Reporte que deberá contener:
   * Introducción
   * Manual técnico (Diseño, desarrllo, componentes)
4. Pantallasos de la implementación generada.