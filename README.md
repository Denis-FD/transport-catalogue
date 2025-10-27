# Transport Catalogue (C++) <!-- omit in toc -->

Каталог остановок и автобусных маршрутов с JSON-вводом/выводом, SVG-картой и поиском кратчайших путей по графу ([алгоритм Дейкстры](https://ru.wikipedia.org/wiki/Алгоритм_Дейкстры), учёт ожидания и скорости).

![Пример SVG-карты маршрутов](assets/map-demo.svg)

## Содержание <!-- omit in toc -->
- [Возможности](#возможности)
- [Архитектура](#архитектура)
- [Сборка](#сборка)
- [Запуск](#запуск)
- [Формат данных](#формат-данных)
  - [Заполнение транспортного каталога (`base_requests`)](#заполнение-транспортного-каталога-base_requests)
  - [Настройки отрисовки карты (`render_settings`)](#настройки-отрисовки-карты-render_settings)
  - [Настройки маршрутизации (`routing_settings`)](#настройки-маршрутизации-routing_settings)
  - [Запросы к каталогу (`stat_requests`)](#запросы-к-каталогу-stat_requests)
- [Примеры](#примеры)
- [Лицензия](#лицензия)

## Возможности
- Импорт базы: остановки, расстояния, маршруты ([`base_requests`](#заполнение-транспортного-каталога-base_requests)).
- Запросы ([`stat_requests`](#запросы-к-каталогу-stat_requests)):
  - **Bus** — длина, извилистость, кол-во остановок/уникальных.
  - **Stop** — маршруты через остановку.
  - **Route** — кратчайший путь (Wait/Bus шаги, общее время).
  - **Map** — SVG-карта.
- Настройки: [`render_settings`](#настройки-отрисовки-карты-render_settings) (карта), [`routing_settings`](#настройки-маршрутизации-routing_settings) (скорость и ожидание).
- Работа через Command Line Interface (**CLI**): чтение из `stdin`, вывод в `stdout`.
- Без сторонних библиотек.

## Архитектура
![Архитектура Transport Catalogue](assets/architecture-diagram.svg)

* **`main.cpp`** — точка входа: инициализация, чтение `stdin`, вывод `stdout`.
* **`json_reader`** — парсинг входных данных и вызов фасада.
* **`request_handler`** — фасад, связывающий все слои приложения.
* **`transport_catalogue`** — хранение остановок, маршрутов и расстояний.
* **`transport_router`**, **`router`**, **`graph`** — построение графа и поиск маршрутов.
* **`map_renderer`** — генерация SVG-карты маршрутов.
* **`json`**, **`json_builder`**, **`svg`** — внешние библиотеки для работы с форматами.
* **`domain`**, **`geo`** — базовые сущности и геометрия.
* **`ranges`** — утилиты для работы с коллекциями.

## Сборка
**Требования:** компилятор с C++17 (GCC, Clang, MinGW).

```bash
g++ -std=c++17 src/*.cpp -o transport-catalogue
```
- **Релиз:** `-O2 -DNDEBUG`
- **Отладка:** `-g -O0 -Wall -Wextra`

## Запуск
```bash
./transport-catalogue < input.json > output.json
```
Программа читает входные данные из `stdin` и выводит результат в `stdout`.

## Формат данных

Во входном JSON ожидаются разделы:

* `base_requests` — данные об остановках и маршрутах
* `render_settings` — параметры отрисовки SVG-карты
* `routing_settings` — параметры построения маршрутов
* `stat_requests` — запросы к транспортному каталогу

### Заполнение транспортного каталога (`base_requests`)

Массив объектов двух типов — `Stop` и `Bus`.
Сначала добавляются все остановки, затем маршруты.

**Stop**

| Поле                    | Тип    | Назначение                          |
| ----------------------- | ------ | ----------------------------------- |
| `type`                  | string | `"Stop"`                            |
| `name`                  | string | Название остановки                  |
| `latitude`, `longitude` | double | Координаты                          |
| `road_distances`        | object | Соседние остановки и расстояния (м) |

```json
{
  "type": "Stop",
  "name": "Рынок",
  "latitude": 55.611087,
  "longitude": 37.20829,
  "road_distances": { "Больница": 1500, "Университет": 3000 }
}
```

**Bus**

| Поле           | Тип    | Назначение                     |
| -------------- | ------ | ------------------------------ |
| `type`         | string | `"Bus"`                        |
| `name`         | string | Название маршрута              |
| `stops`        | array  | Список остановок               |
| `is_roundtrip` | bool   | `true`, если маршрут кольцевой |

```json
{
  "type": "Bus",
  "name": "14",
  "stops": ["Рынок", "Больница", "Университет", "Рынок"],
  "is_roundtrip": true
}
```

### Настройки отрисовки карты (`render_settings`)

| Поле                   | Тип            | Назначение                          |
| ---------------------- | -------------- | ----------------------------------- |
| `width`, `height`      | double         | Размеры карты                       |
| `padding`              | double         | Отступ от краёв                     |
| `stop_radius`          | double         | Радиус кружков остановок            |
| `line_width`           | double         | Толщина линий маршрутов             |
| `bus_label_font_size`  | int            | Размер шрифта маршрутов             |
| `bus_label_offset`     | array          | Смещение подписи маршрута `[x, y]`  |
| `stop_label_font_size` | int            | Размер шрифта остановок             |
| `stop_label_offset`    | array          | Смещение подписи остановки `[x, y]` |
| `underlayer_color`     | string / array | Цвет подложки                       |
| `underlayer_width`     | double         | Толщина подложки                    |
| `color_palette`        | array          | Цвета маршрутов                     |

```json
{
  "width": 1200,
  "height": 600,
  "padding": 50,
  "stop_radius": 5,
  "line_width": 2,
  "bus_label_font_size": 20,
  "bus_label_offset": [7, 15],
  "stop_label_font_size": 10,
  "stop_label_offset": [5, -5],
  "underlayer_color": [255, 255, 255, 0.85],
  "underlayer_width": 3,
  "color_palette": ["green", "red", "blue"]
}
```

### Настройки маршрутизации (`routing_settings`)

| Поле            | Тип    | Назначение                       |
| --------------- | ------ | -------------------------------- |
| `bus_wait_time` | int    | Ожидание на остановке (мин)      |
| `bus_velocity`  | double | Средняя скорость автобуса (км/ч) |

```json
{
  "bus_wait_time": 6,
  "bus_velocity": 40
}
```

### Запросы к каталогу (`stat_requests`)

Массив запросов к каталогу: `Stop`, `Bus`, `Route`, `Map`.

| Поле   | Тип    | Назначение                            |
| ------ | ------ | ------------------------------------- |
| `id`   | int    | Уникальный идентификатор              |
| `type` | string | `"Stop"`, `"Bus"`, `"Route"`, `"Map"` |
| `name` | string | Название (кроме `"Map"`)              |

```json
[
  { "id": 1, "type": "Stop", "name": "Рынок" },
  { "id": 2, "type": "Bus", "name": "14" },
  { "id": 3, "type": "Map" }
]
```

## Примеры

## Лицензия
MIT — см. файл [LICENSE](LICENSE).
