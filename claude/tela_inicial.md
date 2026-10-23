Agora após o login, crie um menu lateral responsivo, também crie um menu em cima ondé sera possivel visualizar o perfil do usuário.

lembre o menu de cima vai ter tamanho fixo, o lateral vai poder ser ocultado e expandido.

opções do menu lateral:
 - cadastro
   - Cliente
   - Funcionario
   - Empresa
 - estoque
   - Cadastrar Produto
   - Consultar Produto
 - Financeiro
   - Registrar compra
   - Registrar venda

no menu de cima ao clicar no usuário deve apresentar uma nova janela, lembre de iniciar com um loading,
pois as informações do usuário vão ser retornadas pelo servidor, crie um metodo que gera uma chave rsa pois vamos utilizar para se comunicar com o servidor na troca de mensagem criticas, como por exemplo as informações do usuário, lembre que deve ser feito o hand shake, enviando a chave publica (enviada) e retendo a chave publica (recebida), você pode armazenar a chave publica em um diretorio criado pela aplicação, lembre de salvar as duas chaves publicas de forma que der para entender qual é a chave publica do cliente/servidor, a chave privada deixe em memoria pois ela só vai servir quando o usuário se logar, pois iremos utilizar a senha informado por ele para criar o rsa, a senha criada por ele vai ser utilizada também quando for salvar algo no computador utilizando eas-256-cbc.

as criptografias simetrica e assimetrica deixe em uma pasta security, crie também metodos para hashing sha256, sha512, o tamanho padrão de chave rsa que vamos utilizar é o 8192.
