# Boull

Cliente em desenvolvimento para Minecraft Java 1.8.9 no Windows x64, com janela nativa para AutoClicker, RightClicker, HitDelayFix, NoJumpDelay, ArrayList, Notifications e Watermark. Lunar e Badlion são os alvos iniciais; a compatibilidade depende da versão do cliente e deve ser verificada no jogo. O módulo SkinDerp ainda não está implementado.

O bind pertence ao AutoClicker e começa em F6. Os clickers exigem que o botão correspondente esteja pressionado e o jogo esteja em foco. O RightClicker aguarda a verificação de uso de itens antes de liberar o botão, limitando a taxa efetiva a aproximadamente 14 CPS. Os módulos dependentes do mapeamento ficam indisponíveis quando os campos necessários não são encontrados.

## Compilação

Requisitos: Visual Studio com ferramentas C++ para Windows x64, CMake 3.25 ou superior e JDK x64.

```powershell
$env:JAVA_HOME = 'C:\caminho\para\o\jdk'
cmake -S . -B build -A x64 -DAUTOCLICKV_BUILD_TESTS=ON
cmake --build build --config Release
```

O executável de distribuição é `build\Release\Boull.exe`. A DLL é incorporada ao executável durante o build. Na primeira execução, o Boull extrai uma cópia verificada em `%LOCALAPPDATA%\Boull\Runtime\<versão>\BoullRuntime.dll`, pois o carregamento pelo Windows utiliza um arquivo. Não é necessário distribuir uma DLL ao lado do executável.

Abra o Minecraft e inicie o Boull. O carregador usa a primeira janela de Minecraft compatível que encontrar. Para descarregá-lo, clique na engrenagem do menu e selecione Unload. O Alt Manager salva apelidos locais para perfis offline; saia do mundo antes de trocar de perfil. Ele não autentica contas Microsoft.

## Atualizações

Ao abrir, `Boull.exe` consulta a última release pública de `Velobounce/Boull` no GitHub. Quando existe uma versão estável mais recente, ele baixa o arquivo `Boull.exe`, confere o SHA-256 informado pela API do GitHub e a versão do executável, substitui o arquivo e reinicia automaticamente. Sem conexão ou sem release pública, a versão atual continua abrindo. A verificação de hash usa os dados recebidos do GitHub e não substitui uma assinatura digital independente.

Para uma release futura funcionar, aumente a versão em `CMakeLists.txt`, compile novamente, publique uma release estável com tag no formato `vMAJOR.MINOR.PATCH` correspondente à versão compilada e anexe o executável com o nome exato `Boull.exe`. O GitHub precisa fornecer o campo `digest` SHA-256 do arquivo. Mantenha esse nome ao distribuir ou executar o programa: o atualizador só substitui um arquivo chamado `Boull.exe`.

## Testes locais

```powershell
.\build\Release\BoullUpdateTest.exe
.\build\Release\BoullHudTest.exe
.\build\Release\AutoClickVClickTest.exe
powershell -NoProfile -ExecutionPolicy Bypass -File .\tests\Run-MappingTest.ps1
```

Os testes exercitam o mapeamento JNI com classes simuladas, o agendamento de cliques, a interface, a integridade do executável e uma substituição de executável em uma pasta de teste. Não substituem a validação em clientes reais nem uma publicação real no GitHub.

## Licenças

O código do projeto está sob a licença MIT em `LICENSE`. As licenças das dependências estão em `third_party/imgui/LICENSE.txt`, `third_party/nlohmann/LICENSE.MIT` e `src/hud/INTER-LICENSE.txt`.


