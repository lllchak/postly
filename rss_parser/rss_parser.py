from asyncio import sleep
from random import uniform
from logging import Logger
from typing import (
    Any,
    Deque,
)

from httpx import (
    AsyncClient,
    Response,
)

from utils import get_header


async def rss_parser(
    httpx_client: AsyncClient,
    source: str,
    rss_url: str,
    deadline_prop: int = 2,
    logger: Logger = None,
) -> None:
    while True:
        try:
            resp: Response = await httpx_client.get(url=rss_url, headers=get_header())
            resp.raise_for_status()
        except Exception as e:
            if logger:
                logger.error(f'Failed to get {source} data: {e}')

            await sleep(deadline_prop*2 - uniform(0, 0.5))
            continue

        print(resp.text)
