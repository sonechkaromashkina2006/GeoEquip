import json
import sys
from neo4j import GraphDatabase

# 1. Параметры подключения к Neo4j
NEO4J_URI = "bolt://localhost:7687"
NEO4J_USER = "neo4j"
NEO4J_PASSWORD = "your_password"  # Замените на ваш пароль

# 2. Логика сопоставления (какое оборудование к какому центру относится)
EQUIPMENT_MAPPING = {
    "Инжиниринговый центр ННГУ": [
        "3D-сканер Creaform HandySCAN 3D",
        "Вертикально-фрезерный обрабатывающий центр Haas VF-2"
    ],
    "Самарский научный центр РАН": [
        "Профилометр Taylor Hobson Surtronic",
        "Сканирующий электронный микроскоп Tescan Vega"
    ]
}

# 3. Cypher-запрос для MERGE узлов (создает узел, только если его нет)
MERGE_NODE_QUERY = """
UNWIND $nodes AS item
CALL {
    WITH item
    WITH item WHERE 'Enterprise' IN item.node.labels
    MERGE (e:Enterprise {name: item.node.properties.name})
    ON CREATE SET 
        e.score = item.node.properties.score,
        e.cost = item.node.properties.cost,
        e.accessibility = item.node.properties.accessibility,
        e.latitude = item.node.properties.latitude,
        e.longitude = item.node.properties.longitude,
        e.description = item.node.properties.description
    ON MATCH SET
        e.score = item.node.properties.score,
        e.cost = item.node.properties.cost,
        e.accessibility = item.node.properties.accessibility

  UNION ALL

    WITH item
    WITH item WHERE 'Equipment' IN item.node.labels
    MERGE (eq:Equipment {name: item.node.properties.name})
    ON CREATE SET 
        eq.description = item.node.properties.description,
        eq.embedding = item.node.properties.embedding
}
"""

# 4. Cypher-запрос для MERGE связей HAS_EQUIPMENT
MERGE_RELATIONSHIP_QUERY = """
UNWIND $links AS link
MATCH (e:Enterprise {name: link.enterprise})
MATCH (eq:Equipment {name: link.equipment})
MERGE (e)-[r:HAS_EQUIPMENT]->(eq)
"""


def load_json_data(file_path: str):
    """Загрузка и базовый парсинг JSON файла."""
    try:
        with open(file_path, "r", encoding="utf-8") as f:
            return json.load(f)
    except json.JSONDecodeError as e:
        print(f"Ошибка чтения JSON (проверьте закрывающие скобки): {e}")
        sys.exit(1)
    except FileNotFoundError:
        print(f"Файл {file_path} не найден.")
        sys.exit(1)


def update_database(json_file_path: str):
    data = load_json_data(json_file_path)

    # Подготавливаем список связей на основе сопоставления
    links = []
    for ent_name, eq_list in EQUIPMENT_MAPPING.items():
        for eq_name in eq_list:
            links.append({"enterprise": ent_name, "equipment": eq_name})

    # Подключение к драйверу Neo4j
    driver = GraphDatabase.driver(NEO4J_URI, auth=(NEO4J_USER, NEO4J_PASSWORD))

    try:
        with driver.session() as session:
            # Шаг 1: Дополняем узлы (Enterprise и Equipment)
            print("Импорт и обновление узлов...")
            session.run(MERGE_NODE_QUERY, nodes=data)

            # Шаг 2: Создаем связи между Enterprise и Equipment
            print("Создание связей HAS_EQUIPMENT...")
            session.run(MERGE_RELATIONSHIP_QUERY, links=links)

            print("Успех! База данных успешно дополнена связями и новыми узлами.")

    except Exception as e:
        print(f"Произошла ошибка при работе с Neo4j: {e}")
    finally:
        driver.close()


if __name__ == "__main__":
    # Укажите путь к вашему исправленному JSON-файлу
    JSON_FILE = "data.json"
    update_database(JSON_FILE)