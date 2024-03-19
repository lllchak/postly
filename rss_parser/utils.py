from logging import (
    INFO,
    Formatter,
    Logger,
    StreamHandler,
)
from random import randint
from typing import Dict
from sys import stdout

from user_agent_list import USER_AGENT_LIST


def get_header() -> Dict[str, str]:
    return {
        'User-Agent': USER_AGENT_LIST[randint(0, len(USER_AGENT_LIST) - 1)],
        'Accept': 'text/html,application/xhtml+xml,application/xml;q=0.9,image/avif,image/webp,*/*;q=0.8',
        'Accept-Language': 'en-US,en;q=0.5',
        'Accept-Encoding': 'gzip, deflate, br',
        'DNT': '1',
        'Sec-Fetch-Dest': 'document',
        'Sec-Fetch-Mode': 'navigate',
        'Sec-Fetch-Site': 'none',
    }


def cretate_logger(
    name: str = 'logger',
    level: int = INFO,
) -> Logger:
    logger: Logger = Logger(name=name, level=level)
    formatter: Formatter = Formatter(
        fmt='%(asctime)s - %(name)s - %(levelname)s \n%(message)s \n' + '-'*30
    )
    handler: StreamHandler = StreamHandler(stream=stdout)

    handler.setFormatter(fmt=formatter)
    logger.addHandler(hdlr=handler)

    return logger

