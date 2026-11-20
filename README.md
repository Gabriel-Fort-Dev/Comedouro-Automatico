
# Comedouro Automático

## Sobre o projeto

O Comedouro Automático é um protótipo desenvolvido com Arduino Uno que monitora o nível de ração por meio de um sensor ultrassônico e controla sua liberação utilizando um servomotor. Possui LEDs, buzzer e display OLED para indicar o estoque e o funcionamento do sistema, proporcionando mais praticidade na alimentação de animais.

## Objetivo

Desenvolver um sistema automatizado capaz de monitorar a quantidade de ração disponível em um reservatório, realizar sua liberação automaticamente e alertar o usuário sobre a necessidade de reposição.

## Componentes utilizados

- Arduino Uno
- Sensor ultrassônico HC-SR04
- Servomotor SG90
- Display OLED 128x64 I2C
- LED verde
- LED amarelo
- LED vermelho
- Buzzer
- Resistores
- Reservatório de ração
- Comporta mecânica

## Funcionamento

O sensor ultrassônico mede a distância entre sua posição e a superfície da ração. O Arduino utiliza essa informação para estimar o nível de estoque e atualizar os indicadores luminosos, o buzzer e o display OLED.

A liberação da ração ocorre por meio de uma comporta acionada pelo servomotor, permitindo que o alimento passe por gravidade. Após o período programado, a comporta é fechada automaticamente.

## Sistema de monitoramento

| Nível | Indicador | Funcionamento |
|---|---|---|
| 80% a 100% | LED Verde | Nível adequado |
| 61% a 79% | LED Amarelo | Atenção para reposição |
| 0% a 60% | LED Vermelho + Buzzer | Nível baixo |
| Leitura inválida | Indicadores desligados | Erro de leitura e comporta fechada |

## Parâmetros do sistema

- Tempo de abertura da comporta: 2 segundos.
- Intervalo mínimo entre liberações: 10 segundos.
- Intervalo de verificação: 200 milissegundos.
- Distância de referência para reservatório cheio: 3 cm.
- Distância de referência para reservatório vazio: 22 cm.

*Os intervalos reduzidos são utilizados na programação atual para testes do protótipo.*

## Tecnologias utilizadas

- Arduino (C/C++)
- Sensoriamento ultrassônico
- Automação eletrônica
- Controle de servomotor
- Comunicação I2C
- Display OLED

## Integrantes

- Gabriel Fortunato de Freitas
- Wellington Safim

**Turma:** TDESS2025V1  
**Instituição:** SENAI  
**Local:** Joinville – SC  
**Ano:** 2026
