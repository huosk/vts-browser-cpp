
var canvas = document.getElementById("canvas")

// canvas size
function updateCanvasSize()
{
    let displayWidth  = canvas.clientWidth
    let displayHeight = canvas.clientHeight
    if (canvas.width  != displayWidth || canvas.height != displayHeight)
    {
        canvas.width  = displayWidth
        canvas.height = displayHeight
    }
}
updateCanvasSize()
window.addEventListener("resize", updateCanvasSize)

// lost opengl context
canvas.addEventListener("webglcontextlost", function()
{
    alert('WebGL context lost. You will need to reload the page.')
})

// collapsible elements
{
    let coll = document.getElementsByClassName("collapsible")
    let i
    for (i = 0; i < coll.length; i++)
    {
        let content = coll[i].nextElementSibling
        content.style.display = "none"
        coll[i].addEventListener("click", function()
        {
            if (content.style.display === "block")
                content.style.display = "none"
            else
                content.style.display = "block"
        });
    }
}

// draggable elements
function draggableElement(elmnt)
{
    var pos1 = 0, pos2 = 0, pos3 = 0, pos4 = 0
    function elementDrag(e)
    {
        e = e || window.event
        e.preventDefault()
        pos1 = pos3 - e.clientX
        pos2 = pos4 - e.clientY
        pos3 = e.clientX
        pos4 = e.clientY
        elmnt.style.top = (elmnt.offsetTop - pos2) + "px"
        elmnt.style.left = (elmnt.offsetLeft - pos1) + "px"
    }
    function closeDragElement()
    {
        document.onmouseup = null
        document.onmousemove = null
    }
    function dragMouseDown(e)
    {
        e = e || window.event
        e.preventDefault()
        pos3 = e.clientX
        pos4 = e.clientY
        document.onmouseup = closeDragElement
        document.onmousemove = elementDrag
    }
    elmnt.querySelector('.header').onmousedown = dragMouseDown
}
{
    let coll = document.getElementsByClassName("draggable")
    let i
    for (i = 0; i < coll.length; i++)
        draggableElement(coll[i])
}

// apply options
var applyOptionsCpp
var Module =
{
    onRuntimeInitialized: function()
    {
        applyOptionsCpp = Module.cwrap("applyOptions", null, ["string"])
        //setTimeout(function(){ applyOptionsCpp('{ "traverseModeGeodata":"none" }') }, 1000)
    },
}
function applyOption(e)
{
    let t = e.target
    applyOptionsCpp('{ "' + t.name + '":"' + t.value + '" }')
}
{
    let coll = document.getElementsByClassName("options")
    let i
    for (i = 0; i < coll.length; i++)
        coll[i].onchange = applyOption
}
