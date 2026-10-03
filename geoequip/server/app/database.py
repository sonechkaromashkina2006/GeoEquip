from typing import Optional
from neo4j import AsyncGraphDatabase, AsyncDriver
from app.config import NEO4J_URI, NEO4J_USER, NEO4J_PASSWORD

driver: Optional[AsyncDriver] = None

async def init_db():
    global driver
    driver = AsyncGraphDatabase.driver(NEO4J_URI, auth=(NEO4J_USER, NEO4J_PASSWORD))

async def close_db():
    global driver
    if driver:
        await driver.close()

def get_driver() -> AsyncDriver:
    if not driver:
        raise RuntimeError("Neo4j driver is not initialized")
    return driver