# GeoEquip — Информационно-аналитическая система поиска оборудования

Гибридная геоинформационная система для поиска и анализа промышленного и научного оборудования с использованием векторного поиска (Sentence-BERT) и графовой базы данных Neo4j.

---

## Требования к окружению

### Backend & Database

* **Docker / Docker Desktop** (или Docker внутри WSL2)
* **Python 3.10+**

### C++ Client

> ⚠️ **КРИТИЧЕСКОЕ ТРЕБОВАНИЕ К СБОРКЕ C++ КЛИЕНТА:**
> Модуль **Qt WebEngineView** в ОС Windows корректно поддерживается и собирается **исключительно под компилятором MSVC** (Microsoft Visual C++). Сборка через MinGW **официально не поддерживается** разработчиками Qt.
>
> Проект гарантированно собирается и работает **строго в следующей связке**:
>
> * **Qt Version:** `Qt 6.9.3`
> * **Compiler:** `MSVC 2022 64-bit` (Microsoft Visual Studio 2022 C++ Build Tools)
> * **Build Target / Kit:** `Desktop Qt 6.9.3 MSVC2022 64bit (Debug/Release)`

---

## Быстрый старт

### 1. Настройка базы данных и бэкенда

1. Перейдите в директорию сервера:

```bash
cd server
```

2. Создайте файл конфигурации `.env` на основе шаблона и укажите параметры подключения:

```bash
cp .env.example .env
```

*Пример содержимого `.env`:*

```ini
NEO4J_URI=bolt://localhost:7687
NEO4J_USER=neo4j
NEO4J_PASSWORD=your_password
```

3. Запустите контейнер Neo4j:

```bash
docker-compose up -d
```

4. Установите зависимости Python:

```bash
pip install -r requirements.txt
```

5. Заполните базу данных объектами, векторными эмбеддингами и постройте граф связей:

```bash
python -m scripts.upload_data
python -m scripts.add_graph_relations
```

6. Запустите FastAPI сервер:

```bash
uvicorn app.main:app --reload --host 0.0.0.0 --port 8000
```

---

### 2. Настройка и сборка C++ Клиента (Qt Creator)

1. Убедитесь, что в **Visual Studio Installer** установлен компонент **Desktop development with C++** (MSVC v143).
2. Установите **Qt 6.9.3** с обязательными компонентами:

* `Qt WebEngine`
* `MSVC 2022 64-bit`

3. Откройте **Qt Creator**.
4. Выберите `File` -> `Open File or Project...` и укажите файл `client/CMakeLists.txt`.
5. На этапе выбора комплекта (Kit) выберите строго **Desktop Qt 6.9.3 MSVC2022 64bit**.
6. Нажмите **Configure Project** и запустите проект (`Ctrl + R`).
