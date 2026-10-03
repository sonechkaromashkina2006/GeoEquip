import json
from sentence_transformers import SentenceTransformer
from neo4j import GraphDatabase

URI = "bolt://localhost:7687"
AUTH = ("NEO4J_USER", "NEO4J_PASSWORD") 

model = SentenceTransformer('paraphrase-multilingual-MiniLM-L12-v2')

from pathlib import Path
import json

SCRIPT_DIR = Path(__file__).resolve().parent
DATA_FILE = SCRIPT_DIR / 'data.json'

with open(DATA_FILE, 'r', encoding='utf-8') as file:
    data = json.load(file)

EQUIPMENT_MAPPING = {
    "Лаборатория 3D-прототипирования КФУ": [
        {"name": "Промышленный 3D-принтер 3D Systems ProX SLS 600", "description": "Селективное лазерное спекание пластиков, изготовление функциональных прототипов"},
        {"name": "3D-сканер Creaform HandySCAN 3D", "description": "Высокоточное лазерное сканирование объектов, обратный инжиниринг, контроль геометрии"}
    ],
    "Инжиниринговый центр ННГУ": [
        {"name": "Вертикально-фрезерный обрабатывающий центр Haas VF-2", "description": "Высокоточная ЧПУ обработка металлов, фрезерование деталей, штампов"},
        {"name": "Профилометр Taylor Hobson Surtronic", "description": "Измерение шероховатости поверхности и волнистости деталей"}
    ],
    "Самарский научный центр РАН": [
        {"name": "Сканирующий электронный микроскоп Tescan Vega", "description": "Исследование микроструктуры материалов, элементный анализ, металлография"},
        {"name": "Рентгеновский дифрактометр Rigaku", "description": "Рентгеноструктурный анализ кристаллических материалов и фазового состава"}
    ],
    "Технопарк «Жигулёвская долина»": [
        {"name": "Токарный станок DMG MORI CTX beta 800", "description": "Высокоточная токарная обработка, ЧПУ точение валов, фланцев и сложных деталей"},
        {"name": "Координатно-измерительная машина Hexagon", "description": "Высокоточные 3D измерения геометрии сложных промышленных изделий"}
    ],
    "Саратовский государственный технический университет": [
        {"name": "Лазерный технологический комплекс чистки и резки", "description": "Волоконный лазер для резки металлов и очистки поверхностей от коррозии"},
        {"name": "Испытательная машина Instron 5982", "description": "Физико-механические испытания материалов на растяжение, сжатие и изгиб"}
    ],
    "Ульяновский наноцентр ULNANOTECH": [
        {"name": "Установка плазмохимического осаждения PECVD", "description": "Нанесение тонких наноразмерных пленок и покрытий в вакууме"},
        {"name": "Атомно-силовой микроскоп NT-MDT", "description": "Визуализация рельефа поверхности с нанометровым разрешением"}
    ],
    "Волгоградский государственный технический университет": [
        {"name": "Роботизированный сварочный комплекс KUKA", "description": "Автоматическая дуговая и лазерная сварка крупногабаритных конструкций"},
        {"name": "Ультразвуковой дефектоскоп Olympus", "description": "Неразрушающий контроль качества сварных соединений и литья"}
    ],
    "Пензенский государственный университет": [
        {"name": "Станция пайки и ремонта BGA компонентов Ersa", "description": "Монтаж и ремонт сложных электронных плат, микросхем и модулей"},
        {"name": "Осциллограф Keysight InfiniiVision", "description": "Анализ высокочастотных электронных сигналов, отладка микропроцессорных систем"}
    ]
}

def upload_all_data():
    with GraphDatabase.driver(URI, auth=AUTH) as driver:
        driver.verify_connectivity()
        #--------------------------------------------------------------------------------------------
        def insert_enterprise_and_equipment(tx, ent_name, item, eq_list_with_embeddings):
            tx.run("""
                MERGE (e:Enterprise {name: $name})
                SET e.lat = $lat,
                    e.lon = $lon,
                    e.equipment_score = $equipment_score,
                    e.accessibility = $accessibility,
                    e.cost = $cost
            """, 
            name=ent_name, 
            lat=item.get('lat', 0.0), 
            lon=item.get('lon', 0.0),
            # Используем .get(), проверяя варианты имен или ставя 0.0 по умолчанию:
            equipment_score=item.get('equipment') or item.get('equipment_score', 0.0), 
            accessibility=item.get('accessibility', 0.0), 
            cost=item.get('cost', 0.0))
            
            for eq in eq_list_with_embeddings:
                tx.run("""
                    MATCH (e:Enterprise {name: $ent_name})
                    MERGE (eq:Equipment {name: $eq_name})
                    SET eq.description = $description,
                        eq.embedding = $embedding
                    MERGE (e)-[:HAS_EQUIPMENT]->(eq)
                """, 
                ent_name=ent_name,
                eq_name=eq['name'],
                description=eq['description'],
                embedding=eq['embedding'])
        #--------------------------------------------------------------------------------------------
        with driver.session() as session:
            for item in data:
                ent_name = item['name']
                eq_list = EQUIPMENT_MAPPING.get(ent_name, [])
                
                # 1. Готовим данные: добавляем векторы прямо в словари оборудования
                prepared_eq_list = []
                for eq in eq_list:
                    text_to_embed = f"{eq['name']}. {eq['description']}"
                    vector = model.encode(text_to_embed).tolist()
                    
                    # Создаем копию словаря и добавляем туда ключ 'embedding'
                    eq_with_vector = eq.copy()
                    eq_with_vector['embedding'] = vector
                    prepared_eq_list.append(eq_with_vector)
                    
                # 2. Отправляем в базу чистые данные без задержек на нейросеть
                session.execute_write(
                    insert_enterprise_and_equipment, 
                    ent_name, 
                    item, 
                    prepared_eq_list # Передаем список с уже готовыми вевекторами
                )
                print(f"сохранено: {ent_name}")
#--------------------------------------------------------------------------------------------
if __name__ == "__main__":
    upload_all_data()