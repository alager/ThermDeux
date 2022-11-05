

var monday;
var tuesday;
var wednesday;
var thursday;
var friday;
var saturday;
var sunday;

var schedBox;
var buttonHeat;
var buttonCool;
var buttonReturn;
var buttonClose;
var buttonDelete;
var myDiv;

// Add event for when the DOM is loaded
// start the javascript from here
document.addEventListener("DOMContentLoaded", function(event)
{
	window.scrollTo(0, 1);
	// document.documentElement.requestFullscreen();
	// screen.orientation.lock('landscape');
	//
	initButtons();
});

// make the scheduleBox clickable
function initButtons()
{
	monday = document.getElementById( "monday" );		
	tuesday = document.getElementById( "tuesday" );
	wednesday = document.getElementById( "wednesday" );	
	thursday = document.getElementById( "thursday" );	
	friday = document.getElementById( "friday" );		
	saturday = document.getElementById( "saturday" );	
	sunday = document.getElementById( "sunday" );		
	
	buttonHeat = document.getElementById( "buttonHeat" );
	buttonCool = document.getElementById( "buttonCool" );
	buttonDelete = document.getElementById( "buttonDelete" );
	buttonReturn = document.getElementById( "buttonReturn" );
	buttonClose = document.getElementById( "buttonClose" );

	schedBox = document.getElementById( "scheduleBox" );
	schedBox.addEventListener( "click", editSched );

	buttonHeat.addEventListener( 'click', addHeat );
	buttonCool.addEventListener( 'click', addCool );
	buttonReturn.addEventListener( 'click', returnButton );
	buttonClose.addEventListener( 'click', closeButton );
}

// shrink the divs and go back to the week view
function returnButton()
{
	schedBox.addEventListener( "click", editSched );
	showDayDivs();

	// make all the divs small again
	myDiv.classList.remove( "dayBig" );
	myDiv.classList.add( "daySmall" );

	// hide the return button
	buttonReturn.hidden = true;
	
	// show the close button
	buttonClose.hidden = false;
}

// close the scheduler and go back to the thermostat view
function closeButton()
{
	// close the div, or reload index....
}

function editSched( event )
{
	// remove this event for now
	schedBox.removeEventListener( "click", editSched );

	// find the day div if the hour target was clicked
	myDiv = event.target;
	// console.log( myDiv.id );

	if( myDiv.id.charAt( 3 ) == '-' )
	{
		// we need to find the parent
		myDiv = myDiv.parentNode;
		// console.log( "new Div: " + myDiv.id );
	}

	// hide all the divs and then only show the one that was clicked
	hideDayDivs();
	myDiv.hidden = false;

	// show the return button
	buttonReturn.hidden = false;

	// hide the close button
	buttonClose.hidden = true;

	// make this div large
	myDiv.classList.add( "dayBig" );
	myDiv.classList.remove( "daySmall" );

}

function hideDayDivs(  )
{
	monday.hidden = true;
	tuesday.hidden = true;
	wednesday.hidden = true;
	thursday.hidden = true;
	friday.hidden = true;
	saturday.hidden = true;
	sunday.hidden = true;
}

function showDayDivs(  )
{
	monday.hidden = false;
	tuesday.hidden = false;
	wednesday.hidden = false;
	thursday.hidden = false;
	friday.hidden = false;
	saturday.hidden = false;
	sunday.hidden = false;
}


function addHeat()
{
	var dotType;
	var newDiv = document.createElement("div");
	newDiv.draggable = true;

	if( myDiv.childNodes[1].childNodes.length > 1 )
	{
		return;
	}
	
	if( myDiv.childNodes[1].childNodes.length > 0 )
	{
		dotType = "heatDotSmall";
		myDiv.childNodes[1].childNodes[0].classList.remove( "coolDotNormal" );
		myDiv.childNodes[1].childNodes[0].classList.add( "coolDotSmall" );
	}
	else
	{
		dotType = "heatDotNormal";
	}
	newDiv.classList.add( dotType );

	myDiv.childNodes[1].appendChild(newDiv);
	// myDiv.childNodes[1].childNodes[0].classList.add( "heatDotNormal");

}

function addCool()
{
	var dotType;
	var newDiv = document.createElement("div");
	newDiv.draggable = true;
	
	if( myDiv.childNodes[1].childNodes.length > 1 )
	{
		return;
	}

	if( myDiv.childNodes[1].childNodes.length > 0 )
	{
		dotType = "coolDotSmall";
		myDiv.childNodes[1].childNodes[0].classList.remove( "heatDotNormal" );
		myDiv.childNodes[1].childNodes[0].classList.add( "heatDotSmall" );
	}
	else
	{
		dotType = "coolDotNormal";
	}
	newDiv.classList.add( dotType );

	// the child node will be something like 'mon-am-00'
	myDiv.childNodes[1].appendChild(newDiv);
	// myDiv.childNodes[1].childNodes[0].classList.add( "coolDotNormal");
}