# RiegaBot: riego automático con estadísticas en Google Drive

os presento un proyecto con Arduino consistente en un riego automático con envío de datos a Google Drive y avisos a dispositivo móvil.

Otras características: control del nivel del depósito de agua, ajustes de la humedad mínima y máxima que se desea mantener en la planta, visualización en tiempo real de la humedad en la planta...

## Vídeo explicativo

En el siguiente enlace puede ver un vídeo explicativo del proyecto:

* [Vídeo](https://www.youtube.com/watch?v=5mBlglAQT7E)

## Contenido

En este repositorio puede encontrar lo siguiente:

* Código del proyecto.

## Información adicional

* Pieza (impresión 3D) para sujetar el tubo a la maceta: https://www.thingiverse.com/thing:313258
* Applet IFTTT para estadísticas del riego: 
   - If Maker Event "riega_log", then Add row to xxxxxxx@gmail.com’s Google Drive spreadsheet
   - Event name: riega_log
   - Spreadsheet name: registro_regabot
   - Formatted row: {{OccurredAt}} ||| {{EventName}} ||| {{Value1}} |||{{Value2}}|||{{Value3}}
* Applet IFTTT para mensajes al móvil: 
   - If Maker Event "nivel_deposito", then Send a notification from the IFTTT app
   - Event name: nivel_deposito
   - Message: RiegaBot: el nivel del depósito es del {{Value1}} % 
   - Debe descargar la app de IFTTT en su dispositivo móvil, disponible en App Store y Google Play.

## Contribuir

**¡Toda contribución es bienvenida!**

Usa las opciones que proporciona GitHub: **Pull Request** o **Fork** para colaborar con el proyecto.

**Fork**: Realiza un clon de este repositorio en tu cuenta de GitHub. Podrás hacer modificaciones o simplemente para tener una copia (con opción de clonarlo a tu PC también). De esa forma garantizas la información para tu uso personal.

**Pull Request**: Así me envías la sugerencia de cambio para este repositorio, que previamente has realizado en tu clon. Si todo va bien, serán aceptadas :)

## Autor

- David Fdez. Mtnez. [@davidfdezmtnez](https://twitter.com/davidfdezmtnez) 

## Licencia

Bajo MIT License - ver [LICENSE](LICENSE) para más detalles.
