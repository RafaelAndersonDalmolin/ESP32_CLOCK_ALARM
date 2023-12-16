### **Driver de Debouncing Tratando Sinais de Interrupção de Botões Push**

********Finalidade:********

O driver de debouncing é uma parte essencial para lidar com sinais de interrupção provenientes de botões push conectados a pinos específicos de um microcontrolador ESP32. O objetivo desse driver é garantir que os sinais de entrada sejam interpretados corretamente, eliminando qualquer ruído ou flutuações que possam ocorrer quando um botão é pressionado ou liberado. Além disso, o driver oferece funcionalidades avançadas, permitindo a identificação de três tipos de eventos: clique simples, clique longo e clique com repetição automática.

**Funcionalidades existentes:**

1. **Debouncing:** O debouncing é o processo de filtrar transições rápidas e instáveis que podem ocorrer quando um botão é pressionado ou liberado. O driver de debouncing utiliza algoritmos para suavizar o sinal e garantir que apenas eventos de botão válidos sejam registrados.
2. **Identificação de Clique Simples:** O driver é capaz de detectar e identificar um clique simples quando o botão é pressionado e liberado rapidamente. Isso é útil para ações como selecionar uma opção ou executar uma função.
3. **Identificação de Clique Longo:** Além do clique simples, o driver permite a detecção de um clique longo quando o botão é mantido pressionado por um período definido. Isso pode ser utilizado para funcionalidades como ajustes contínuos.
4. **Identificação de Clique com Repetição Automática:** O driver também oferece suporte para identificar cliques com repetição automática quando o botão é mantido pressionado continuamente. Essa funcionalidade é útil para rolagem contínua ou outras ações que requerem entrada contínua.

**Utilização do Driver de Debouncing para ESP32: Configuração Inicial**

Antes de utilizar o driver de debouncing para ESP32, é necessário realizar algumas configurações iniciais que determinarão o comportamento do driver ao lidar com os botões conectados. As configurações incluem:

**1. Quantidade de Botões a serem Instalados:** Informe o número total de botões que serão conectados ao microcontrolador ESP32. 

**2. Tempo de Debounce:** Defina o tempo de debounce desejado. Este é o período durante o qual o driver ignora quaisquer alterações no estado do botão após uma mudança, a fim de evitar falsas interrupções devido a ruído elétrico. Esse valor deve ser configurado de acordo com as características elétricas do sistema e do ambiente.

**3. Tempo de Polling:** Determine o intervalo de tempo em que o driver monitorará continuamente o nível lógico do botão. Isso ajuda a garantir uma detecção precisa das ações do botão.

**4. Tempo de Pressão para Click Longo:** Especifique o tempo que o botão deve ser mantido pressionado para ser considerado um clique longo. 

**5. Tempo de Pressão para Ativar o Auto Repeat:** Defina o tempo que o botão deve ser mantido pressionado para ativar a função de clique com repetição automática (Autoclick).

**6. Tempo de Intervalo para o Auto Repeat:** Especifique o intervalo de tempo entre as ações de Autoclick. Isso determina a frequência com que a ação é repetida enquanto o botão é mantido pressionado.

**Parâmetros do Usuário para a Instalação de Botões:**

Ao instalar cada botão, o usuário deve fornecer os seguintes parâmetros como argumentos para a instalação do driver:

1. **GPIO Acoplado ao Botão:** Indique o número GPIO ao qual o botão está conectado.
2. **Utilização de Resistores Pull-donw/Pull-up Internos:** Especifique se serão utilizados resistores internos para o botão.
3. **Nível Lógico do Botão Quando Pressionado (High ou Low):** Determine se o nível lógico do pino do botão é alto (HIGH) ou baixo (LOW) quando o botão é pressionado.
4. **Utilização da Função Especial Autoclick:** Especifique se deseja habilitar a função de autoclick para o botão. Isso permite a repetição automática da ação quando o botão é mantido pressionado.
5. **Utilização da Função Especial Pressed Long:** Determine se deseja habilitar a função de clique longo para o botão. Isso permite a detecção de cliques longos.
    
    **OBS: é possível definir apenas uma função especial para cada botão.** 
    
6. **Número do Pino para Fila Compartilhada (-1 para Exclusivo ou NUM_GPIO para Compartilhada):** Se definido como -1, sera criado uma fila de uso exclusivo para o botão; caso contrário, ela será compartilhado com outros botões do pino passado como argumento. Isso permite que você consuma as informações da fila para tomar ações específicas com base nos eventos identificados.

Ao seguir essas diretrizes de configuração inicial e fornecer os parâmetros adequados para cada botão, o driver de debouncing para ESP32 será configurado para lidar com os botões de acordo com suas necessidades específicas, permitindo uma interação precisa e confiável com o dispositivo.

********Diagrama elétrico de montagem dos botões:********

[diagrama botoes.pdf](./diagrama_botoes.pdf)