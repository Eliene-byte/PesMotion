# Exemplos

`chute_frontal.bvh` é um esqueleto sintético de 2 ossos (raiz + coxa),
criado à mão para demonstração — não é captura real e não reivindica realismo;
serve apenas para validar o pipeline:

```sh
pesmotion info examples/chute_frontal.bvh
pesmotion convert --in examples/chute_frontal.bvh --out /tmp/chute.pmca
pesmotion build --profile pes2021 --out /tmp/PesMotion_PES2021.bin /tmp/chute.pmca
```
