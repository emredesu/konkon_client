# konkon_client

<img src="https://i.imgur.com/LauBN2N.png"/>

### konkon_client is the GUI client for the chat application Konkon.
[Server repository](https://github.com/emredesu/konkon_server)

#### Requirements:
* C++17 (or higher)
* Qt 5.12.10 (or higher)
* [Boost](https://www.boost.org/)
* [nlohmann](https://github.com/nlohmann/json)
* Icon and sfx files (get them from releases)

#### Building on Windows (Visual Studio)
1- Get QT VS Tools, set it up with your existing Qt installation

2- Jam all the files in a Visual Studio project

3- Set up include and library paths and files for Boost and nlohmann libraries

4- If you are connecting to a real domain that has its own certificate file, add the certificate file in the directory and edit [connectionmanager.cpp](https://github.com/emredesu/konkon_client/blob/01b166b10e96601503ce6b103ffb9d0b6c7b23ac/src/connectionmanager.cpp#L9) accordingly.

5- Build!

#### Building on Linux
1- ¯\\_(ツ)_/¯ (todo)

### Screenshots

<img src="https://i.imgur.com/bW21gBN.png">
<img src="https://i.imgur.com/rhJWsG9.png">
<img src="https://i.imgur.com/Z9JAPnO.png">

##### Certificate files included are signed locally and included to be able to run the server locally. They are not secure to use.
