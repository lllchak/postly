from locust import HttpUser, task, constant
import random
from pyquery import PyQuery as pq

class TTest(HttpUser):
    wait_time: int = constant(2)

    @task
    def threads(self):
        language: str = self.__get_lang()
        category: str = self.__get_category()
        period: int = random.randrange(10, 100)
        self.client.get(
            url=f"/threads?period={period}&lang_code={language}&category={category}",
            auth=None,
            headers={"Cache-Control": "max-age=100"},
        )

    def __get_lang(self):
        return random.choice(["en", "ru"])

    def __get_category(self):
        categories: list = [
            "society",
            "sports",
            "entertainment",
            "science",
            "technology",
            "other",
            "economy"
        ]
        return random.choice(categories)
