# 🌌 Aetherius

<div align="center">

**Бескомпромиссная производительность. Архитектура нового поколения.**
*Высокопроизводительное ядро Minecraft сервера на C++ с поддержкой C# и Luau.*

</div>

---

## 🚧 Репозиторий и Доступ

> **Внимание:** На данный момент репозиторий [SocialQuestDev/aetherius](htttps://github.com/SocialQuestDev/aetherius) находится в режиме **Public Alpha**. Доступ предоставляется только по приглашениям. Публичный релиз запланирован в Roadmap.

---

## ⚡ Why Aetherius? (Философия Архитектуры)

Minecraft индустрия годами ограничена рамками JVM. **Aetherius** устраняет фундаментальные боттлнеки за счет отказа от классической Java-архитектуры:

1. **Zero-Cost Abstractions:** C++20 обеспечивает полный контроль над памятью. Забудьте о непредсказуемых паузах **Garbage Collector (GC)** при выгрузке тысяч чанков.
2. **Data-Oriented Design:** Использование SIMD-инструкций и Cache-friendly структур данных для обработки сущностей.
3. **True Multithreading:** Архитектура без глобальных блокировок (Lock-free). Логика мира, физика и сетевой I/O работают в параллельных потоках изначально.
4. **Modern Scripting:** Интеграция Luau и C# позволяет писать логику любой сложности без потери производительности.

---

## 🚀 Ключевые Возможности

### 🛠 Core & Performance

* **Native C++20 Implementation:** Минимальный оверхед и прямой доступ к системным вызовам.
* **Asynchronous I/O:** Высокопроизводительный сетевой стек на базе `Asio`.
* **Modular Engine:** Ядро разделено на изолированные сервисы (Network, World, ECS, Scripting).

### 🔌 Hybrid Plugin System

Уникальная система расширений, сочетающая гибкость и мощь:

1. **Luau (High-Level Scripting):**
* Форк Lua от Roblox с постепенной типизацией и высокой скоростью выполнения.
* **Hot-reload:** Изменяйте логику сервера или Хуков на лету без перезагрузки сервера.
* Безопасная песочница для пользовательских скриптов.


2. **C# .NET 10+ (Enterprise Grade):**
* Использование мощностей современного .NET вместо тяжеловесного JNI/Java.
* Высокая скорость разработки сложных систем (экономика, БД, интеграции).
* Низкий порог входа для разработчиков, привыкших к типизированным языкам.



---

## 🛠 Технологический Стек

| Категория | Технология | Описание |
| --- | --- | --- |
| **Language** | **C++20** | Основной язык ядра (Concepts, Coroutines). |
| **Scripting** | **Luau** | Быстрый скриптинг с проверкой типов. |
| **Managed SDK** | **.NET 10+ / C#** | Основная среда для сложных плагинов. |
| **Networking** | **Asio / OpenSSL** | Асинхронная обработка пакетов и шифрование. |
| **Data** | **NBT / Protobuf** | Сериализация данных. |
| **Architecture** | **EnTT (ECS)** | Система сущностей для обработки тысяч мобов. |

---

## 🗺 Roadmap

Мы движемся итеративно. Текущий статус: **Phase 2**.

### **Phase 1: Foundation (Complete)**

* [x] Инициализация проекта и кроссплатформенная сборка (CMake).
* [x] Реализация базового Handshake и Login протокола (1.16.5+).

### **Phase 2: World & Interaction (Current)**

* [x] Стриминг чанков (Anvil/Region format).
* [x] Базовая физика и синхронизация позиций.
* [ ] Интеграция **Luau VM** в жизненный цикл тиков.

### **Phase 3: The Hybrid Engine**

* [ ] API для Luau (События, работа с миром).
* [ ] Интеграция **Hostfxr** для запуска C# плагинов внутри процесса.
* [ ] Поддержка мульти-версионности (1.8 - 1.2x).

### **Phase 4: Release Candidate**

* [ ] Оптимизация AI и системы ECS.
* [ ] Публичная документация и SDK для плагинов.
* [ ] Open Source релиз.

---

## 💻 Getting Started (For Contributors)

### Требования

* **Compiler:** GCC 11+, Clang 14+ или MSVC 2022.
* **Tools:** CMake 3.22+, Git.
* **Environment:** .NET 10 SDK (для разработки и сборки C# модулей).

### Сборка проекта (🐧 Linux)

```bash
# 1. Клонирование
git clone https://github.com/SocialQuestDev/aetherius.git
cd aetherius

# 2. Обновление зависимостей (Luau, Asio, и др.)
git submodule update --init --recursive

# 3. Сборка
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
cmake --build . --parallel

```

### Запуск

```bash
./bin/aetherius_server --config server.json

```

---

<div align="center">

Developed with ❤️ and C++ by **[wexels.dev](https://github.com/wexelsdev)**, **[glitching.today](https://github.com/todayisglitching)**, **[Pawmi](https://github.com/pawmii)**, && Aetherius contributors.
*Revolutionizing the blocky world.*

</div>
