# STX - Systems Toolbelt for C++23

STX es una librería header-only de bajo nivel diseñada para el desarrollo de herramientas de sistemas, análisis de ejecutables y manipulación eficiente de datos binarios. La librería aprovecha las características del estándar C++23 para proporcionar una API moderna con el rendimiento de C.

## Especificaciones Técnicas
* **Estándar:** C++23 (requiere soporte para `std::to_underlying`, `std::is_constant_evaluated` y otras caracteristicas).
* **Arquitectura:** Header-only.
* **Gestión de Memoria:** Optimizada para evitar zero-initialization en buffers temporales.

---

## Módulos y Funcionalidades

### 1. Memoria (mem.hpp)
Implementa el contenedor `dirty_vector<T>`, una especialización de `std::vector` que utiliza un allocator personalizado (`vec_init_allocator`).
* **dirty_vector<T>**: A diferencia de la implementación estándar, este contenedor realiza una inicialización por defecto (default-initialization) en lugar de una inicialización por valor (value-initialization). Esto elimina el coste de CPU de rellenar la memoria con ceros cuando se conoce que los datos serán sobrescritos inmediatamente.

### 2. Sistema de Archivos (fs.hpp)
Proporciona una interfaz simplificada para operaciones de entrada/salida binaria sobre `std::istream`.
* **readfs<Type>(file, offset, dir)**: Lee una estructura de tamaño fijo en una posición específica.
* **readfs<Type>(file, offset, count, dir)**: Lee múltiples elementos directamente a un `dirty_vector<Type>`.
* **setposfs(file, offset, dir)**: Wrapper de `seekg` que utiliza el tipo `offset_t` para mayor seguridad de tipos.

### 3. Núcleo y Tipos (core.hpp / fn.hpp)
* **Alias de Tipos**: Definiciones de ancho fijo (`u8`, `u32`, `i64`, `usize`, etc.).
* **Casting**: Implementaciones de `scast`, `rcast` y `ccast` como sustitutos legibles de los casts de C++.
* **Conceptos**: Definición del concepto `binary_readable` para restringir la lectura de archivos a tipos POD y estructuras compatibles con memoria contigua.

### 4. Utilidades (time.hpp / range.hpp)
* **to_time**: Convierte Timestamps de 32 bits (Win32/PE) en objetos `std::chrono` legibles.
* **range**: Generador de secuencias para iteración simplificada en bucles `for`.

---

## Ejemplo de Implementación: Parser de Cabeceras PE

El siguiente código ilustra cómo integrar los módulos para procesar un ejecutable de Windows.

```cpp
#include <stx/stx.hpp>
#include <fstream>

using namespace stx;

auto main() -> int
{
    std::ifstream file { "target.dll", std::ios::binary };
    if ( not file.is_open() ) return 1;

    // Lectura de cabeceras fijas
    auto dos_header { readfs<IMAGE_DOS_HEADER  >( file ) };
    auto nt_header  { readfs<IMAGE_NT_HEADERS64>( file, offset_t{ dos_header.e_lfanew }) };

    // Cálculo de offset alineado para la tabla de secciones
    auto sections_offset { offset_t { 0
        + dos_header.e_lfanew
        + sizeof( u32 )
        + sizeof( IMAGE_FILE_HEADER )
        + nt_header.FileHeader.SizeOfOptionalHeader
    }};

    // Lectura masiva a buffer optimizado (sin zero-fill)
    auto sections { readfs<IMAGE_SECTION_HEADER>(
        file,
        sections_offset,
        nt_header.FileHeader.NumberOfSections
    )};

    for ( const auto& section : sections ) {
        println("Seccion: {}", section.get_name());
    }

    return EXIT_SUCCESS;
}
```

---

## Integración mediante CMake

Para utilizar STX como una dependencia de interfaz en su proyecto:

```cmake
# CMakeLists.txt
add_subdirectory(extern/stx)
target_link_libraries( <target> PRIVATE stx::stx )
```

