# Autoaprendizaje

Módulo nativo de AzerothCore basado en el Lua del servidor. Las tablas de clase,
equitación y tótems están en `src/AutoAprendizajeData.inc`. El módulo no requiere
ALE ni migraciones SQL.

1. Regenera la solución de AzerothCore y compila `worldserver`.
2. Copia `conf/mod-auto-aprendizaje.conf.dist` como
   `configs/modules/mod-auto-aprendizaje.conf` junto a la instalación de worldserver.
3. Retira `Mod_Auto_Aprendizaje.lua` del directorio de scripts activos de ALE.
4. Configura `AutoAprendizaje.Enable = 1` y reinicia con el nuevo worldserver.

El módulo empieza desactivado para evitar que se ejecute junto con el Lua.
Lee la configuración al iniciar y al recargarla, y aplica el aprendizaje al
entrar, subir de nivel o aprender un talento. Las competencias adicionales son
opcionales y respetan los niveles definidos por la lista de hechizos de clase.
