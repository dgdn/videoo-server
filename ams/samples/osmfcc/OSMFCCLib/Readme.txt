OSMFCCDecoder is the primary method to enable closed captioning with the OSMF Player.
OSMFCCDecoder takes care of parenting the displayObject, updating the video size,
and resetting the decoder:

 >Enabling/disabling caption display 
 >Selection of a desired caption "service" (i.e. a specific caption channel) 
 >Override of various caption display attributes (e.g. font, text and background colors, etc.). 

*************************************************************************
Using OSMFCCDecoder to enable closed captioning in your application consists of the following steps:
 a>Prepare your assets
 b>Initialize the decoder
 c>Expose customizable attributes
  
*************************************************************************
 a>Prepare your assets
 
 In order to take advantage of OSMFCCDecoder, your assets must be
 packaged to encapsulate closed captioning data in the onCaptionInfo
 message. 
 
 In AMS please ensure GenerateCCInfo is enabled in Server.xml for vod playback
 <Root>
	<Server>
		<Streams>
 			<Mp4>
				<!-- mp4: Playback specific configurations -->
				<Playback>
					<!-- When enabled, AMS will process the video packets for closed caption and   -->
					<!-- generate onCaptionInfo message to be sent to the client. 					       -->
					<!-- This overrides this setting.												                       -->
					<GenerateCCInfo>true</GenerateCCInfo>
				</Playback> 
			</Mp4>
		</Streams>
	</Server>
 </Root>
			
 And for live streaming GenerateCCInfo is enabled in Application.xml 
 <Application>
	<StreamManager>
		<Live>
			<!-- When enabled, AMS will process the video packets for closed caption and       -->
			<!-- generate onCaptionInfo message to be sent to the client. 					-->					
			<GenerateCCInfo>true</GenerateCCInfo>
		</Live>
	 </StreamManager>	
</Application>	 

 For more information on this area, please consult your packager or encoder vendor.
 
*************************************************************************
b>Initialize the Decoder
 
Initializing the OSMFCCDecoder typically consists of a few steps, as illustrated in the following code:
 
 <Action Script Code>
     var player:MediaPlayer = ...;
 
     // ... initialize the decoder when both player and view is ready...

     var ccDecoder:OSMFCCDecoder;

     // create a new instance
     ccDecoder = new OSMFCCDecoder();
     
     // bind the instance to your media player
     ccDecoder.mediaPlayer = player;
 
     // bind the instance to the media player view area which can be used to render (e.g. mediaContainer)
     ccDecoder.mediaContainer = mediaContainer;
 
     // enable the instance
     ccDecoder.enabled = true;

     // optionally select a type and service
     //At present, only one type is supported; CEA-608 over 708, which is identified by the value CCType.CEA708.
     ccDecoder.type = CCType.CEA708;
     ccDecoder.service = CEA708Service.CC1;
	
     //Details of all the ActionsCript API exposed by OSMFCCDecoder is detailed below
	
 </Action Script Code>
 
 Note: You should have 1 instance of OSMFCCDecoder per video player.
 
*************************************************************************
c>Expose customizable attributes
 
 OSMFCCDecoder contains a number of properties that should be exposed via user interface controls. 
 Most of these properties enable overriding font and color attributes and are required under FCC guidelines. 
 You are responsible for adding UI controls to expose these properties, as well adding the necessary code that will save and restore settings
 between sessions. 
 * UI controls in the video player must be provided to enable/disable closed captioning display and to select the desired caption service.
 * UI controls may be provided to allow the end user to select override values for the displayed font and text and background color and opacity.
 
 ***************************ActionScript OSMFDecoder APIs**********************************************

     /**
     * The type of closed captioning data to decode and render. A value of null indicates no selected type. 
     * Valid values are values of the CCType enumeration class.
     * At present, only one type is supported; CEA-608 over 708, which is identified by the value CCType.CEA708.
     * The default value is null.
     *
     * @throws ArgumentError if the specified type is not supported.
     */
    public function get type():String;
    public function set type(value:String):void;

    /**
     * The MediaPlayer this OSMFCCDecoder is associated with.
     * Assigning any value disassociates the OSMFCCDecoder from a currently assigned MediaPlayer (if any).
     * If the assigned value is non-null, the OSMFCCDecoder will automatically register itself as an event listener on
     * the provided MediaPlayer and access its view property to use as the video surface to render captions over.
     */
    public function get mediaPlayer():MediaPlayer
    public function set mediaPlayer(value:MediaPlayer):void

    /**
     * Whether caption data decoding and rendering is enabled or disabled.
     * The default value is true.
     * When this property transitions from true to false a side-effect is a reset of internal decoder state and
     * any currently rendered captions are cleared.
     */
    public function get enabled():Boolean
    public function set enabled(value:Boolean):void

    /**
     * The caption "service" to decode and render.
     * For CEA-608 over 708 closed captioning, supported values are values of the CEA708Service enumeration class.
     * CEA708Service.CC1, CEA708Service.CC2, CEA708Service.CC3, CEA708Service.CC4
     * @throws ArgumentError if the assigned service value is not supported or available for the closed captioning
     *         type specified at construction time.
     */
    public function get service():String
    public function set service(value:String):void

    /**
     * Font override to apply to rendered text.
     * Default value is null, indicating no override, and text will assume its intrinsic font according to caption
     * authoring.
     * To override the font assign an embedded or system font name and to disable an override assign null.
     */
    public function get font():String
    public function set font(value:String):void

    /**
     * Text color override to apply to rendered text.
     * Default value is -1, indicating no override, and text will assume its intrisic color which may vary according
     * to caption authoring.
     * To override the text color assign a RGB hex value (e.g. 0xFF00FF) and to disable an override assign -1.
     * @throws ArgumentError if the value is not -1 and not within the RGB hex range.
     */
    public function get textColor():int
    public function set textColor(value:int):void

    /**
     * Background color override to apply to the background of text character cells.
     * Default value is -1, indicating no override, and cell backgrounds will assume their intrinsic color which
     * may vary according to caption authoring.
     * To override the background color assign a RGB hex value (e.g. 0xFF00FF) and to disable an override assign -1.
     * @throws ArgumentError if the value is not -1 and not within the RGB hex range.
     */
    public function get backgroundColor():int
    public function set backgroundColor(value:int):void

    /**
     * Text opacity override to apply to rendered text.
     * Default value is -1, indicating no override, and text will assume its intrinsic opacity which may vary
     * according to caption authoring.
     * To override text opacity assign a value between 0 (fully transparent) to 1 (fully opaque) and to disable
     * an override assign -1.
     * @throws ArgumentError if the value is not -1 and not within the range of [0, 1].
     */
    public function get textOpacity():Number
    public function set textOpacity(value:Number):void

    /**
     * Background opacity override to apply to the background of text character cells.
     * Default value is -1, indicating no override, and backgrounds will assume their intrinsic opacity which may
     * vary according to caption authoring.
     * To override background opacity assign a value between 0 (fully transparent) to 1 (fully opaque) and to
     * disable an override assign -1.
     * @throws ArgumentError if the value is not -1 and not within the range of [0, 1].
     */
    public function get backgroundOpacity():Number
    public function set backgroundOpacity(value:Number):void

    /**
     * Resets the OSMFCCDecoder's internal state, clearing any currently rendered captions.
     * The currently selected caption service and display overrides are not changed.
     */
    public function reset():void

    /**
     * The default font to use when no custom font is specified.
     * This is set to a reasonable default for the device. When using custom devices,
     * you may alter this property to change the default font.
     */
    public function get defaultFont():String
    public function set defaultFont(value:String):void
		
    /**
     * Edge type override to apply to rendered text.
     * 
     * Valid values are indicated by the EdgeType enumeration, or null to
     * indicate no override. Valid Values: 
     * EdgeType.NONE: Indicates that no edge decoration should be used.
     * EdgeType.DEPRESSED: Indicates that the edge will appear as depressed.
     * EdgeType.UNIFORM: Indicates that the edge will with a uniform border.
     * EdgeType.LEFT_DROP_SHADOW: Indicates that the edge will with a shadow falling to the left.
     * EdgeType.RIGHT_DROP_SHADOW: Indicates that the edge will with a shadow falling to the right.
     *
     * @default null
     * 
     * @throws ArgumentError if the value is not null and not a memory of the EdgeType enumeration.
     **/
    public function get edgeType():String
    public function set edgeType(value:String):void


    /**
     * Edge color override to apply to rendered text. The value should be an RGB hex value (e.g. 0xFF00FF).
     * 
     * A value of -1 indicates no override. A reasonable default edge color will be selected.
     * 
     * @default -1
     * 
     * @throws ArgumentError if the value is not -1 and not within the RGB hex range.
     **/
    public function get edgeColor():int
    public function set edgeColor(value:int):void
