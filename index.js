// Main Alexa SDK and AWS SDK imports
const Alexa = require('ask-sdk-core');
const AWS   = require('aws-sdk');

// ---------- AWS IoT and DynamoDB configuration ----------
const IotData = new AWS.IotData({
    // AWS IoT endpoint for this account / region
    endpoint: 'a1acybki981kqw-ats.iot.us-east-2.amazonaws.com'
});

// DynamoDB client used to map Alexa user → IoT thing
const dynamodb   = new AWS.DynamoDB.DocumentClient();
const TABLE_NAME = 'user_thing';

// ================== HELPERS ==================

/**
 * Resolve the IoT thing name associated with the current Alexa user.
 * 1. Reads the Alexa userId from the request.
 * 2. Queries DynamoDB table `user_thing` using user_id as partition key.
 * 3. If a mapping exists, returns the stored thing_name.
 * 4. If not, falls back to the default thing "iot_thing".
 */
async function getThingName(handlerInput) {
    const userId = handlerInput.requestEnvelope.session.user.userId;

    console.log("=== USER ID DE ALEXA (getThingName) ===");
    console.log(userId);
    console.log("=======================================");

    const params = {
        TableName: TABLE_NAME,
        Key: { user_id: userId }
    };

    console.log('Buscando thing para user:', userId);

    try {
        const data = await dynamodb.get(params).promise();
        if (data.Item && data.Item.thing_name) {
            console.log('Thing encontrado:', data.Item.thing_name);
            return data.Item.thing_name;
        } else {
            // Fallback when no explicit mapping is found for the user
            console.log('No se encontró thing para este usuario, usando por defecto iot_thing');
            return 'iot_thing';          // <- must match the actual IoT thing name
        }
    } catch (err) {
        // On read error we still fall back to the default thing
        console.log('Error leyendo de DynamoDB:', err);
        return 'iot_thing';
    }
}

/**
 * Retrieve the full AWS IoT Device Shadow for the given thing.
 * Logs both the raw shadow and the state subsection for debugging.
 * Returns `payload.state` (object with desired/reported sections) or null on error.
 */
async function getShadow(thingName) {
    const params = { thingName };

    console.log('Obteniendo shadow para thing:', thingName);

    try {
        const data       = await IotData.getThingShadow(params).promise();
        const payloadStr = data.payload.toString();
        const payload    = JSON.parse(payloadStr);

        console.log("===== SHADOW COMPLETO =====");
        console.log(JSON.stringify(payload, null, 2));
        console.log("===== SOLO STATE =====");
        console.log(JSON.stringify(payload.state, null, 2));

        if (payload.state) {
            const desired  = payload.state.desired  || {};
            const reported = payload.state.reported || {};

            console.log(">>> ESTADOS DE PUERTAS (DESIRED) <<<");
            console.log("interiorDoor (desired):",  desired.interiorDoor);
            console.log("exteriorDoor (desired):",  desired.exteriorDoor);

            console.log(">>> ESTADOS DE PUERTAS (REPORTED) <<<");
            console.log("interiorDoor (reported):", reported.interiorDoor);
            console.log("exteriorDoor (reported):", reported.exteriorDoor);
        }

        console.log("=====================================");

        return payload.state;
    } catch (err) {
        // Any failure while reading / parsing the shadow is logged and null is returned
        console.log('Error al obtener el shadow:', err);
        return null;
    }
}

/**
 * Update the desired state of `interiorDoor` in the thing's shadow.
 * `newState` is typically "OPEN" or "CLOSE".
 * This does not wait for the device to actually move; it only updates the shadow.
 */
async function setInteriorDoorDesired(thingName, newState) {
    const payload = {
        state: {
            desired: {
                interiorDoor: newState
            }
        }
    };

    console.log("Actualizando shadow de", thingName, "a:", newState);

    const params = {
        thingName,
        payload: JSON.stringify(payload)
    };

    try {
        const data = await IotData.updateThingShadow(params).promise();
        console.log("updateThingShadow OK:", data);
    } catch (err) {
        console.log("Error en updateThingShadow:", err);
    }
}

// ================== HANDLERS ==================

// Handles the initial "open skill" request (LaunchRequest)
const LaunchRequestHandler = {
    canHandle(handlerInput) {
        return Alexa.getRequestType(handlerInput.requestEnvelope) === 'LaunchRequest';
    },
    handle(handlerInput) {
        console.log("=== USER ID DE ALEXA (LaunchRequest) ===");
        console.log(handlerInput.requestEnvelope.session.user.userId);
        console.log("========================================");

        const speakOutput =
          'Bienvenido a tu objeto inteligente. ' +
          'Puedes abrir la puerta interior, cerrarla o consultar su estado. ¿Qué deseas hacer?';

        return handlerInput.responseBuilder
            .speak(speakOutput)
            .reprompt(speakOutput) // Keep session open waiting for a follow-up intent
            .getResponse();
    }
};

// Handles intent to OPEN the interior door (updates desired state in the shadow)
const OpenInteriorDoorIntentHandler = {
    canHandle(handlerInput) {
        return Alexa.getRequestType(handlerInput.requestEnvelope) === 'IntentRequest'
            && Alexa.getIntentName(handlerInput.requestEnvelope) === 'OpenInteriorDoorIntent';
    },
    async handle(handlerInput) {
        console.log("=== USER ID DE ALEXA (OpenInteriorDoorIntent) ===");
        console.log(handlerInput.requestEnvelope.session.user.userId);
        console.log("=================================================");

        const thingName = await getThingName(handlerInput);
        await setInteriorDoorDesired(thingName, "OPEN");

        const speakOutput = 'Solicitaste abrir la puerta interior.';

        return handlerInput.responseBuilder
            .speak(speakOutput)
            .reprompt('¿Quieres hacer algo más?')
            .getResponse();
    }
};

// Handles intent to CLOSE the interior door (updates desired state in the shadow)
const CloseInteriorDoorIntentHandler = {
    canHandle(handlerInput) {
        return Alexa.getRequestType(handlerInput.requestEnvelope) === 'IntentRequest'
            && Alexa.getIntentName(handlerInput.requestEnvelope) === 'CloseInteriorDoorIntent';
    },
    async handle(handlerInput) {
        console.log("=== USER ID DE ALEXA (CloseInteriorDoorIntent) ===");
        console.log(handlerInput.requestEnvelope.session.user.userId);
        console.log("==================================================");

        const thingName = await getThingName(handlerInput);
        await setInteriorDoorDesired(thingName, "CLOSE");

        const speakOutput = 'Solicitaste cerrar la puerta interior.';

        return handlerInput.responseBuilder
            .speak(speakOutput)
            .reprompt('¿Quieres hacer algo más?')
            .getResponse();
    }
};

// Handles queries for the interior door state (reads from shadow desired/reported)
const StateInteriorDoorHandler = {
    canHandle(handlerInput) {
        return Alexa.getRequestType(handlerInput.requestEnvelope) === 'IntentRequest'
            && Alexa.getIntentName(handlerInput.requestEnvelope) === 'StateInteriorDoorIntent';
    },
    async handle(handlerInput) {
        console.log("=== USER ID DE ALEXA (StateInteriorDoorIntent) ===");
        console.log(handlerInput.requestEnvelope.session.user.userId);
        console.log("==================================================");

        const thingName   = await getThingName(handlerInput);
        const shadowState = await getShadow(thingName);

        // Default error message if we cannot read the shadow
        let speakOutput = 'No pude obtener el estado de la puerta interior. Intenta de nuevo más tarde.';

        if (shadowState) {
            const desired  = shadowState.desired  || {};
            const reported = shadowState.reported || {};
            // Prefer reported; if not present fall back to desired
            const value = reported.interiorDoor || desired.interiorDoor;

            console.log('Estado interiorDoor desde shadow:', value);

            if (value === 'OPEN') {
                speakOutput = 'La puerta interior está abierta.';
            } else if (value === 'CLOSE' || value === 'CLOSED') {
                speakOutput = 'La puerta interior está cerrada.';
            } else {
                speakOutput = 'La puerta interior tiene un estado desconocido en el dispositivo.';
            }
        }

        return handlerInput.responseBuilder
            .speak(speakOutput)
            .reprompt('¿Quieres consultar algo más?')
            .getResponse();
    }
};

// Handles queries for the exterior door state (reads from shadow desired/reported)
const StateExteriorDoorHandler = {
    canHandle(handlerInput) {
        return Alexa.getRequestType(handlerInput.requestEnvelope) === 'IntentRequest'
            && Alexa.getIntentName(handlerInput.requestEnvelope) === 'StateExteriorDoorIntent';
    },
    async handle(handlerInput) {
        console.log("=== USER ID DE ALEXA (StateExteriorDoorIntent) ===");
        console.log(handlerInput.requestEnvelope.session.user.userId);
        console.log("==================================================");

        const thingName   = await getThingName(handlerInput);
        const shadowState = await getShadow(thingName);

        // Default error message if we cannot read the shadow
        let speakOutput = 'No pude obtener el estado de la puerta exterior. Intenta de nuevo más tarde.';

        if (shadowState) {
            const desired  = shadowState.desired  || {};
            const reported = shadowState.reported || {};
            // Prefer reported; if not present fall back to desired
            const value = reported.exteriorDoor || desired.exteriorDoor;

            console.log('Estado exteriorDoor desde shadow:', value);

            if (value === 'OPEN') {
                speakOutput = 'La puerta exterior está abierta.';
            } else if (value === 'CLOSE' || value === 'CLOSED') {
                speakOutput = 'La puerta exterior está cerrada.';
            } else {
                speakOutput = 'La puerta exterior tiene un estado desconocido en el dispositivo.';
            }
        }

        return handlerInput.responseBuilder
            .speak(speakOutput)
            .reprompt('¿Quieres hacer otra consulta?')
            .getResponse();
    }
};

// ================== GENERIC / BUILT-IN HANDLERS ==================

/**
 * Standard Help intent. Explains to the user what the skill can do.
 */
const HelpIntentHandler = {
    canHandle(handlerInput) {
        return Alexa.getRequestType(handlerInput.requestEnvelope) === 'IntentRequest'
            && Alexa.getIntentName(handlerInput.requestEnvelope) === 'AMAZON.HelpIntent';
    },
    handle(handlerInput) {
        const speakOutput =
          'Puedes decir: abre la puerta interior, cierra la puerta interior, ' +
          'o pregunta por el estado de la puerta interior o exterior. ¿Qué deseas hacer?';

        return handlerInput.responseBuilder
            .speak(speakOutput)
            .reprompt('¿Qué deseas hacer?')
            .getResponse();
    }
};

/**
 * Handles both Cancel and Stop built-in intents, ending the conversation.
 */
const CancelAndStopIntentHandler = {
    canHandle(handlerInput) {
        return Alexa.getRequestType(handlerInput.requestEnvelope) === 'IntentRequest'
            && (
                Alexa.getIntentName(handlerInput.requestEnvelope) === 'AMAZON.CancelIntent' ||
                Alexa.getIntentName(handlerInput.requestEnvelope) === 'AMAZON.StopIntent'
            );
    },
    handle(handlerInput) {
        const speakOutput = 'Hasta pronto.';

        return handlerInput.responseBuilder
            .speak(speakOutput)
            .getResponse();
    }
};

/**
 * Fallback handler for utterances that do not match any defined intent.
 */
const FallbackIntentHandler = {
    canHandle(handlerInput) {
        return Alexa.getRequestType(handlerInput.requestEnvelope) === 'IntentRequest'
            && Alexa.getIntentName(handlerInput.requestEnvelope) === 'AMAZON.FallbackIntent';
    },
    handle(handlerInput) {
        const speakOutput =
          'Lo siento, no entendí. Puedes pedir abrir o cerrar la puerta interior, ' +
          'o preguntar por el estado de la puerta interior o exterior.';

        return handlerInput.responseBuilder
            .speak(speakOutput)
            .reprompt('¿Qué deseas hacer?')
            .getResponse();
    }
};

/**
 * Session end handler. Used to log the reason when Alexa closes the session.
 */
const SessionEndedRequestHandler = {
    canHandle(handlerInput) {
        return Alexa.getRequestType(handlerInput.requestEnvelope) === 'SessionEndedRequest';
    },
    handle(handlerInput) {
        console.log(`~~~~ Session ended: ${JSON.stringify(handlerInput.requestEnvelope)}`);
        // No response is required; just return an empty response
        return handlerInput.responseBuilder.getResponse();
    }
};

/**
 * Intent reflector: useful for debugging. It simply repeats the intent name
 * that was triggered. This should remain last in the handler chain.
 */
const IntentReflectorHandler = {
    canHandle(handlerInput) {
        return Alexa.getRequestType(handlerInput.requestEnvelope) === 'IntentRequest';
    },
    handle(handlerInput) {
        const intentName  = Alexa.getIntentName(handlerInput.requestEnvelope);
        const speakOutput = `Intentaste ejecutar ${intentName}.`;

        return handlerInput.responseBuilder
            .speak(speakOutput)
            .getResponse();
    }
};

/**
 * Global error handler. Catches any exception thrown in the skill,
 * logs diagnostic information (including the Alexa userId if available),
 * and returns a generic apology message to the user.
 */
const ErrorHandler = {
    canHandle() {
        return true;
    },
    handle(handlerInput, error) {
        console.log("=== ERROR EN SKILL ===");
        if (handlerInput &&
            handlerInput.requestEnvelope &&
            handlerInput.requestEnvelope.session &&
            handlerInput.requestEnvelope.session.user) {
            console.log("USER ID (en error):", handlerInput.requestEnvelope.session.user.userId);
        }
        console.log(JSON.stringify(error));
        console.log("===================================");

        const speakOutput = 'Disculpa, hubo un problema. Intenta de nuevo.';

        return handlerInput.responseBuilder
            .speak(speakOutput)
            .reprompt('¿Puedes repetir lo que deseas hacer?')
            .getResponse();
    }
};

// ================== EXPORT ==================

// Skill entry point: registers all handlers and exposes the Lambda handler function
exports.handler = Alexa.SkillBuilders.custom()
    .addRequestHandlers(
        LaunchRequestHandler,
        StateExteriorDoorHandler,
        StateInteriorDoorHandler,
        OpenInteriorDoorIntentHandler,
        CloseInteriorDoorIntentHandler,
        HelpIntentHandler,
        CancelAndStopIntentHandler,
        FallbackIntentHandler,
        SessionEndedRequestHandler,
        IntentReflectorHandler
    )
    .addErrorHandlers(ErrorHandler)
    .withCustomUserAgent('sample/hello-world/v1.2')
    .lambda();