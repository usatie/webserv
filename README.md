# webserv

## 例外に関する方針
1. try/catchとthrowの使用を極力少なくする
2. catchの中で使用する関数はnoexcept保証する（`throw()）

### try/catchを使わざるを得ないケース（4箇所）
1. サーバーの初期化 (::main)           -> exit
2. リクエストの処理 (Server::resume)   -> remove_connection
3. コネクションの受付 (Server::accept) -> 何もしない
4. コネクションの受付 (Socket::accept) -> close(fd) : 例外安全の基本保証をするため
