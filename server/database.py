import os
import sqlite3


class Database(object):
    def __init__(self, path, max_messages):
        self.path = path
        self.max_messages = max_messages
        if not os.path.exists(self.path):
            self._send_sql(DB_QUERY_CREATE_TABLE)
    
    def append_message(self, message):
        self._send_sql(DB_QUERY_APPEND_MESSAGE, (message,))
    
    def get_last_messages(self, show_distance=True):
        query = DB_QUERY_GET_MESSAGES if show_distance else DB_QUERY_GET_MESSAGES_NO_DISTANCE
        return self._send_sql(query.format(self.max_messages))

    def reset(self):
        self._send_sql(DB_QUERY_RESET)

    def _send_sql(self, query, params = None):
        with sqlite3.connect(self.path) as conn:
            cursor = conn.cursor()
            if params is None:
                cursor.execute(query)
            else:
                cursor.execute(query, params)
            results = cursor.fetchall()
            conn.commit()
        return results



DB_QUERY_CREATE_TABLE = """
    CREATE TABLE messages (
        id INTEGER PRIMARY KEY AUTOINCREMENT,
        message TEXT NOT NULL,
        timestamp DATETIME DEFAULT (datetime('now', 'localtime'))
    )
"""

DB_QUERY_APPEND_MESSAGE = """
    INSERT INTO messages (message) VALUES (?)
"""

DB_QUERY_GET_MESSAGES = """
    SELECT message, timestamp
        FROM messages
        ORDER BY id DESC
        LIMIT {}
"""

DB_QUERY_GET_MESSAGES_NO_DISTANCE = """
    SELECT message, timestamp
        FROM messages
        WHERE
            message NOT LIKE 'distance:%'
        ORDER BY id DESC
        LIMIT {}
"""

DB_QUERY_RESET = """
    DELETE FROM messages
"""

