from neo4j import GraphDatabase

URI = "bolt://localhost:7687"
AUTH = ("neo4j", "04207148")  
    
def check_connection():
    try:
        with GraphDatabase.driver(URI, auth=AUTH) as driver:
            driver.verify_connectivity()
            print("connected")
            
            with driver.session() as session:
                result = session.run("MATCH (n) RETURN count(n) as count")
                count = result.single()["count"]
                print(f"nodes in database: {count}")
                
    except Exception as e:
        print(f"err: {e}")

if __name__ == "__main__":
    check_connection()