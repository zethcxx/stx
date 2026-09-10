# `ct::record` — mapa `clave → tipo` en tiempo de compilación

> Header: `#include <lbyte/stx/ct/record.hpp>` (u `#include <lbyte/stx/ct.hpp>`)
> Módulo: `import lbyte.stx.ct.record;` (u `import lbyte.stx.ct;`)

Un `record` es un **descriptor de layout** listo para usar *como un struct*: alguna de
sus claves existen en tiempo de compilación, las consultas (`offset_of`, `byte_total`,
`value_of`, …) se resuelven con `constexpr`, y la lectura binaria reparte los bytes
miembro a miembro — igual que haría un `struct` con ese mismo alineado.

`record` es un **tipo vacío**: solo la *tupla de valores* (`value_t`) ocupa memoria en
tiempo de ejecución, y solo cuando la lees. Todo lo demás es puro cálculo de
compilación, cero coste.

## Definición

```cpp
namespace ct = lbyte::stx::ct;

enum class sec : u8 { count, stride, crc };

using info = ct::record<
    ct::member<sec::count,  u16>,
    ct::member<sec::stride, u32>,
    ct::member<sec::crc,    u32>
>;
```

- Cada **`member<Key, Type>`** asocia una clave (un `enum class` o un `off_s`/`rva_s`)
  con un tipo binario (`binary_readable`).
- Todas las claves deben **compartir el mismo tipo** y ser **únicas**; los valores
  deben ser **trivialmente copiables**. Se comprueban con `static_assert`.

## Layout (como un struct)

El alineado por defecto copia las reglas de tu compilador: cada miembro se coloca
en su offset alineado natural (con `alignof` y el mismo `offsetof` que usaría un
`struct` de verdad) y el total incluye el padding final.

```cpp
static_assert( info::count == 3 );

static_assert( info::offsets       [0] == 0 );   // offsets alineados
static_assert( info::offsets       [1] == 2 );
static_assert( info::offsets       [2] == 4 );

static_assert( info::offset_of<sec::crc>() == 4 );   // el "offsetof" de una clave
static_assert( info::index_of<sec::stride>() == 1 );  // posición ordinal

static_assert( info::byte_total == 8 );       // tamaño alineado (trailing pad)
static_assert( info::packed_total == 6 );     // tamaño empaquetado (sin pad)
static_assert( info::max_align == 4 );
```

### Atributos

| Atributo            | Efecto                                                        |
| ------------------- | ------------------------------------------------------------- |
| `attr::packed`      | Layout empaquetado (sin padding), como `#pragma pack`         |
| `attr::align<N>`    | Alinea a `N` (miembro o todo el record)                       |
| `attr::gap<N>`      | Inserta `N` bytes de padding antes del miembro siguiente      |

Se ponen **en el miembro** o **a nivel de record**:

```cpp
using header = ct::record<
    ct::attr::packed,                            // todo el record empaquetado
    ct::member<sec::count, u16>,                 // offset 0, 2 bytes
    ct::attr::gap<6>,                            // 6 bytes de relleno
    ct::member<sec::crc, u32, ct::attr::align<8>> // alineado a 8
>;

// packed: sin padding salvo el gap y el align explícitos
```

## Consultas

```cpp
static_assert( info::has<sec::crc> );                          // existe la clave
static_assert( std::same_as<info::value_of<sec::stride>, u32> ); // tipo de la clave
static_assert( info::key_of<u32>() == sec::stride );           // clave por tipo
```

Todas son `constexpr`, así que puedes usarlas en `static_assert`, como
template-argument, en `if constexpr`… sin pagar nada en runtime.

## Recorrer los miembros

Como cada miembro tiene un **tipo distinto**, la iteración tipada usa
`visit`/`fold` (funciones de compilación), no un `for` de runtime:

```cpp
// visit: una llamada por miembro, en orden
info::visit([](auto m) {
    // 'm' es el ct::member<K,T>; puedes usar typename decltype(m)::value_type, m.key...
    store( m.key );
});

// fold: acumula en orden
constexpr auto total = info::fold(usize{0}, [](auto m, usize acc) {
    return acc + sizeof(typename decltype(m)::value_type);
});
static_assert( total == 6 );
```

`info::members` expone la tupla heterogénea de miembros para `std::get` /
structured bindings:

```cpp
auto&& [c, s, r] = info::members;
```

> Nota: un bucle `for (auto&& m : members)` *no puede* retipar `m` por índice en
> cada vuelta sin reflection; esa es exactamente la labor de `visit`/`fold`.

## Leer y escribir

`value_t` es la tupla de los tipos de valor — *no* tiene el layout del record.

```cpp
info::value_t values{ 1, 0x1000, 0xDEADBEEF };

std::byte raw[info::byte_total]{};
ct::store<info>( std::span<std::byte>(raw), values );   // escribe en offsets

auto back = ct::load<info>( std::span<const std::byte>(raw) );
auto [count, stride, crc] = back;
```

`ct::store<info>(std::byte*, values)` y `ct::store<info>(span<std::byte>, values)`
se comportan igual.

## Como un struct para `ptr` / `memcur`

Con `reader_of`/`is_record` integrados, `ptr` y `memcur` leen un record
**miembro a miembro** — el equivalente de castear tu struct:

```cpp
ptr<std::byte> p{ raw };

auto first  = p.read<info>();  // vuelve la value_t (no avanza)
auto second = p.pop <info>();  // value_t y avanza byte_total

assert( p.addr() == reinterpret_cast<uptr>(raw) + info::byte_total );

memcur cur{ raw, sizeof(raw) };
auto [count, stride, crc] = cur.pop<info>();  // mismo reparto miembro a miembro
```

Esto respeta alineado, packed y gaps de forma idéntica a `ct::load`/`ct::store`.

## Casos de uso típicos

- **Descriptor de layout**: offsets y tamaños sin reservar memoria.
- **Lookup robusto a cambios de orden**: `has`/`value_of`/`index_of` por clave, no por posición.
- **Sembrar offsets de árboles**: `offset_of<K>` + el valor en runtime.
- **Lectura de esquemas/registros binarios**: `store`/`load`, o `pop<rec>` en un
  `memcur` — el orden físico y el lógico se definen una sola vez.

## También

- `record_value_of_t<Rec>` = `Rec::value_t`, útil como acceso genérico.
- Los includes por pieza: `<lbyte/stx/ct/str.hpp>` (strings CT) y
  `<lbyte/stx/ct/record.hpp>` (esto); `<lbyte/stx/ct.hpp>` incluye ambos.